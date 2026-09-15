# based on glm_pp
# https://github.com/jwueller/glm_pp

"""
launch.json for VSCode with cmake plugin installed

{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "(gdb) Launch",
            "type": "cppdbg",
            "request": "launch",
            "program": "${command:cmake.launchTargetPath}",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "text": "source ${workspaceFolder}/contrib/vengi_gdb_pretty_printer.py"
                }
            ]
        }
    ]
}
"""

import gdb
import gdb.printing


def _strip(val):
    if val.type.code in (gdb.TYPE_CODE_REF, gdb.TYPE_CODE_RVALUE_REF):
        val = val.referenced_value()
    return val.cast(val.type.strip_typedefs())


def _as_int(val):
    return int(val.cast(gdb.lookup_type('unsigned long long')))


def _is_null(ptr):
    try:
        if ptr.is_optimized_out:
            return True
        return _as_int(ptr) == 0
    except gdb.error:
        return True


def _print_elements():
    try:
        n = int(gdb.parameter('print elements'))
        if n == 0:
            return 10 ** 9
        return n
    except (gdb.error, TypeError, ValueError):
        return 200


def _template_arg(val, index):
    t = val.type.strip_typedefs()
    if t.code in (gdb.TYPE_CODE_REF, gdb.TYPE_CODE_RVALUE_REF):
        t = t.target().strip_typedefs()
    return t.template_argument(index)


def _template_int(val, index):
    return int(_template_arg(val, index))


def _c_array_len(array_val):
    lo, hi = array_val.type.range()
    return hi - lo + 1


def _iter_n(n):
    limit = min(int(n), _print_elements())
    for i in range(limit):
        yield i


def _vec_info(v):
    # vec contains either a union of structs or a struct of unions, depending on
    # configuration. gdb can't properly access the named members, and in some
    # cases the names are wrong.
    # It would be simple to cast to an array, similarly to how operator[] is
    # implemented, but values returned by functions called from gdb don't have
    # an address.
    # Instead, recursively find all fields of required type and sort by offset.
    #
    # Component count comes from glm::vec<L, T, Q> (template arg 0). sizeof/T
    # is wrong for aligned vec3, which is padded to 16 bytes.

    v = _strip(v)
    T = v.type.template_argument(1)

    if T.code == gdb.TYPE_CODE_FLT:
        if T.sizeof == 4:
            type_prefix = ""
        elif T.sizeof == 8:
            type_prefix = "d"
        else:
            raise NotImplementedError
    elif T.code == gdb.TYPE_CODE_INT:
        if T.is_signed:
            type_prefix = "i"
        else:
            type_prefix = "u"
    elif T.code == gdb.TYPE_CODE_BOOL:
        type_prefix = "b"
    else:
        raise NotImplementedError

    try:
        length = int(v.type.template_argument(0))
    except (gdb.error, TypeError, ValueError):
        length = v.type.sizeof // T.sizeof

    items = {}

    def find(cur, bitpos):
        t = cur.type.strip_typedefs()
        if t.code in (gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION):
            for f in t.fields():
                if hasattr(f, "bitpos"):  # not static
                    find(cur[f], bitpos + f.bitpos)
        elif t == T:
            items[bitpos] = cur

    find(v, 0)
    ordered = [str(f) for _, f in sorted(items.items())]
    if len(ordered) > length:
        ordered = ordered[:length]
    elif len(ordered) < length:
        # Fallback: treat the object as a packed T[length] at offset 0.
        try:
            arr = v.address.cast(T.array(length - 1).pointer()).dereference()
            ordered = [str(arr[i]) for i in range(length)]
        except gdb.error:
            pass
    return type_prefix, length, ordered


class VecPrinter:
    def __init__(self, v):
        self.v = v

    def to_string(self):
        type_prefix, length, items = _vec_info(self.v)
        return "{}vec{}({})".format(type_prefix, length, ", ".join(items))


class MatPrinter:
    def __init__(self, v):
        self.v = v

    def to_string(self):
        V = _strip(self.v)["value"]
        columns = []
        for i in range(V.type.range()[1] + 1):
            type_prefix, length, items = _vec_info(V[i])
            columns.append("({})".format(", ".join(items)))
        return "{}mat{}x{}({})".format(
            type_prefix, len(columns), length, ", ".join(columns))


class StringPrinter:
    def __init__(self, val):
        self.val = val

    def display_hint(self):
        return 'string'

    def to_string(self):
        data = _strip(self.val)['_data']
        ptr = data['_str']
        if _is_null(ptr):
            return ''
        size = _as_int(data['_size'])
        try:
            return ptr.string(encoding='utf-8', length=size)
        except gdb.error:
            try:
                return ptr.string(length=size)
            except gdb.error:
                return str(ptr)


class DynamicArrayPrinter:
    def __init__(self, val):
        self.val = _strip(val)

    def _size(self):
        return _as_int(self.val['_size'])

    def to_string(self):
        size = self._size()
        capacity = _as_int(self.val['_capacity'])
        extra = ''
        try:
            extra = ', increase=%d' % _as_int(self.val['_increase'])
        except gdb.error:
            pass
        return 'core::DynamicArray(size=%d, capacity=%d%s)' % (size, capacity, extra)

    def display_hint(self):
        return 'array'

    def num_children(self):
        return min(self._size(), _print_elements())

    def children(self):
        size = self._size()
        buffer = self.val['_buffer']
        if _is_null(buffer) or size == 0:
            return
        for i in _iter_n(size):
            yield '[%d]' % i, buffer[i]


class BufferPrinter:
    def __init__(self, val):
        self.val = _strip(val)

    def _size(self):
        return _as_int(self.val['_size'])

    def to_string(self):
        return 'core::Buffer(size=%d, capacity=%d)' % (
            self._size(), _as_int(self.val['_capacity']))

    def display_hint(self):
        return 'array'

    def num_children(self):
        return min(self._size(), _print_elements())

    def children(self):
        size = self._size()
        buffer = self.val['_buffer']
        if _is_null(buffer) or size == 0:
            return
        for i in _iter_n(size):
            yield '[%d]' % i, buffer[i]


class ArrayPrinter:
    def __init__(self, val):
        self.val = _strip(val)
        self._items = self.val['_items']
        self._size = _c_array_len(self._items)

    def to_string(self):
        return 'core::Array(size=%d)' % self._size

    def display_hint(self):
        return 'array'

    def num_children(self):
        return min(self._size, _print_elements())

    def children(self):
        for i in _iter_n(self._size):
            yield '[%d]' % i, self._items[i]


class VectorPrinter:
    def __init__(self, val):
        self.val = _strip(val)

    def _size(self):
        return _as_int(self.val['_size'])

    def to_string(self):
        size = self._size()
        cap = _c_array_len(self.val['_items'])
        return 'core::Vector(size=%d, capacity=%d)' % (size, cap)

    def display_hint(self):
        return 'array'

    def num_children(self):
        return min(self._size(), _print_elements())

    def children(self):
        items = self.val['_items']
        for i in _iter_n(self._size()):
            yield '[%d]' % i, items[i]


class StackPrinter:
    def __init__(self, val):
        self.val = _strip(val)

    def _size(self):
        return _as_int(self.val['_size'])

    def to_string(self):
        size = self._size()
        cap = _c_array_len(self.val['_stack'])
        return 'core::Stack(size=%d, capacity=%d)' % (size, cap)

    def display_hint(self):
        return 'array'

    def num_children(self):
        return min(self._size(), _print_elements())

    def children(self):
        stack = self.val['_stack']
        for i in _iter_n(self._size()):
            yield '[%d]' % i, stack[i]


class DynamicStackPrinter:
    def __init__(self, val):
        self.val = _strip(val)
        self._inner = DynamicArrayPrinter(self.val['_stack'])

    def to_string(self):
        size = self._inner._size()
        capacity = _as_int(self._inner.val['_capacity'])
        return 'core::DynamicStack(size=%d, capacity=%d)' % (size, capacity)

    def display_hint(self):
        return 'array'

    def num_children(self):
        return self._inner.num_children()

    def children(self):
        return self._inner.children()


class RingBufferPrinter:
    def __init__(self, val):
        self.val = _strip(val)

    def _size(self):
        return _as_int(self.val['_size'])

    def to_string(self):
        size = self._size()
        cap = _c_array_len(self.val['_buffer'])
        front = _as_int(self.val['_front'])
        return 'core::RingBuffer(size=%d, capacity=%d, front=%d)' % (size, cap, front)

    def display_hint(self):
        return 'array'

    def num_children(self):
        return min(self._size(), _print_elements())

    def children(self):
        size = self._size()
        buffer = self.val['_buffer']
        cap = _c_array_len(buffer)
        front = _as_int(self.val['_front'])
        if cap == 0:
            return
        for i in _iter_n(size):
            yield '[%d]' % i, buffer[(front + i) % cap]


class QueuePrinter:
    def __init__(self, val):
        self.val = _strip(val)

    def _size(self):
        return _as_int(self.val['_size'])

    def to_string(self):
        return 'core::Queue(size=%d, capacity=%d, head=%d, tail=%d)' % (
            self._size(),
            _as_int(self.val['_capacity']),
            _as_int(self.val['_head']),
            _as_int(self.val['_tail']))

    def display_hint(self):
        return 'array'

    def num_children(self):
        return min(self._size(), _print_elements())

    def children(self):
        size = self._size()
        capacity = _as_int(self.val['_capacity'])
        buffer = self.val['_buffer']
        head = _as_int(self.val['_head'])
        if _is_null(buffer) or size == 0 or capacity == 0:
            return
        for i in _iter_n(size):
            yield '[%d]' % i, buffer[(head + i) % capacity]


class MRUBufferPrinter:
    def __init__(self, val):
        self.val = _strip(val)

    def _size(self):
        return _as_int(self.val['_size'])

    def to_string(self):
        size = self._size()
        cap = _c_array_len(self.val['_buffer'])
        return 'core::MRUBuffer(size=%d, capacity=%d)' % (size, cap)

    def display_hint(self):
        return 'array'

    def num_children(self):
        return min(self._size(), _print_elements())

    def children(self):
        buffer = self.val['_buffer']
        for i in _iter_n(self._size()):
            yield '[%d]' % i, buffer[i]


class BufferViewPrinter:
    def __init__(self, val):
        self.val = _strip(val)

    def _size(self):
        return _as_int(self.val['_size'])

    def to_string(self):
        return 'core::BufferView(size=%d)' % self._size()

    def display_hint(self):
        return 'array'

    def num_children(self):
        return min(self._size(), _print_elements())

    def children(self):
        size = self._size()
        buffer = self.val['_buffer']
        if _is_null(buffer) or size == 0:
            return
        for i in _iter_n(size):
            yield '[%d]' % i, buffer[i]


class ListPrinter:
    def __init__(self, val):
        self.val = _strip(val)

    def _size(self):
        try:
            return _as_int(self.val['_allocator']['_currentAllocatedItems'])
        except gdb.error:
            return 0

    def to_string(self):
        return 'core::List(size=%d)' % self._size()

    def display_hint(self):
        return 'array'

    def num_children(self):
        return min(self._size(), _print_elements())

    def children(self):
        node = self.val['_first']
        seen = set()
        for i in _iter_n(self._size()):
            if _is_null(node):
                return
            addr = _as_int(node)
            if addr in seen:
                yield '[cycle]', hex(addr)
                return
            seen.add(addr)
            cur = node.dereference()
            yield '[%d]' % i, cur['value']
            node = cur['next']


class DynamicListPrinter:
    def __init__(self, val):
        self.val = _strip(val)
        try:
            self._elem_type = _template_arg(self.val, 0)
        except gdb.error:
            self._elem_type = None

    def _size(self):
        return _as_int(self.val['_size'])

    def to_string(self):
        return 'core::DynamicList(size=%d)' % self._size()

    def display_hint(self):
        return 'array'

    def num_children(self):
        return min(self._size(), _print_elements())

    def children(self):
        node = self.val['_first']
        seen = set()
        for i in _iter_n(self._size()):
            if _is_null(node):
                return
            addr = _as_int(node)
            if addr in seen:
                yield '[cycle]', hex(addr)
                return
            seen.add(addr)
            cur = node.dereference()
            if self._elem_type is not None:
                storage = cur['valueStorage']
                value = storage.address.cast(self._elem_type.pointer()).dereference()
            else:
                value = cur['valueStorage']
            yield '[%d]' % i, value
            node = cur['next']


def _bucket_items(val):
    buckets = _strip(val)['_buckets']
    try:
        return buckets['_items']
    except gdb.error:
        return buckets


def _walk_hash_entries(val):
    items = _bucket_items(val)
    lo, hi = items.type.range()
    seen = set()
    for b in range(lo, hi + 1):
        entry = items[b]
        while not _is_null(entry):
            addr = _as_int(entry)
            if addr in seen:
                return
            seen.add(addr)
            kv = entry.dereference()
            yield kv
            entry = kv['next']
            if len(seen) >= _print_elements():
                return


class HashMapPrinter:
    def __init__(self, val, name='core::Map', keys_only=False):
        self.val = _strip(val)
        self.name = name
        self.keys_only = keys_only

    def _size(self):
        try:
            return _as_int(self.val['_size'])
        except gdb.error:
            try:
                return _as_int(self.val['_allocator']['_currentAllocatedItems'])
            except gdb.error:
                return 0

    def to_string(self):
        extra = ''
        try:
            extra = ', capacity=%d' % _as_int(self.val['_allocator']['_maxPoolSize'])
        except gdb.error:
            pass
        return '%s(size=%d%s)' % (self.name, self._size(), extra)

    def display_hint(self):
        return 'array' if self.keys_only else 'map'

    def children(self):
        i = 0
        for kv in _walk_hash_entries(self.val):
            if self.keys_only:
                yield '[%d]' % i, kv['key']
            else:
                yield '[%d].key' % i, kv['key']
                yield '[%d].value' % i, kv['value']
            i += 1


class BitSetPrinter:
    def __init__(self, val, name='core::BitSet'):
        self.val = _strip(val)
        self.name = name
        try:
            self._nbits = _template_int(self.val, 0)
        except (gdb.error, TypeError, ValueError):
            buf = self.val['_buffer']
            self._nbits = _c_array_len(buf) * 64

    def _buffer(self):
        return self.val['_buffer']

    def _bit(self, idx):
        buf = self._buffer()
        word = _as_int(buf[idx // 64])
        return (word >> (idx % 64)) & 1

    def to_string(self):
        n = self._nbits
        set_count = 0
        for i in range(n):
            if self._bit(i):
                set_count += 1
        return '%s<%d>(set=%d/%d)' % (self.name, n, set_count, n)

    def display_hint(self):
        return 'array'

    def children(self):
        shown = 0
        limit = _print_elements()
        for i in range(self._nbits):
            if self._bit(i):
                yield '[%d]' % i, i
                shown += 1
                if shown >= limit:
                    return


class DynamicBitSetPrinter:
    def __init__(self, val):
        self.val = _strip(val)

    def _nbits(self):
        return _as_int(self.val['_size'])

    def _bit(self, idx):
        buf = self.val['_buffer']
        if _is_null(buf):
            return 0
        word = _as_int(buf[idx // 64])
        return (word >> (idx % 64)) & 1

    def to_string(self):
        n = self._nbits()
        set_count = 0
        for i in range(n):
            if self._bit(i):
                set_count += 1
        return 'core::DynamicBitSet(set=%d/%d)' % (set_count, n)

    def display_hint(self):
        return 'array'

    def children(self):
        shown = 0
        limit = _print_elements()
        for i in range(self._nbits()):
            if self._bit(i):
                yield '[%d]' % i, i
                shown += 1
                if shown >= limit:
                    return


class Array2DViewPrinter:
    def __init__(self, val):
        self.val = _strip(val)

    def to_string(self):
        w = _as_int(self.val['_width'])
        h = _as_int(self.val['_height'])
        return 'core::Array2DView(width=%d, height=%d)' % (w, h)

    def display_hint(self):
        return 'array'

    def children(self):
        w = _as_int(self.val['_width'])
        h = _as_int(self.val['_height'])
        data = self.val['_data']
        if _is_null(data) or w <= 0 or h <= 0:
            return
        shown = 0
        limit = _print_elements()
        for y in range(h):
            for x in range(w):
                yield '[%d][%d]' % (y, x), data[y * w + x]
                shown += 1
                if shown >= limit:
                    return


class Array3DViewPrinter:
    def __init__(self, val):
        self.val = _strip(val)

    def to_string(self):
        w = _as_int(self.val['_width'])
        h = _as_int(self.val['_height'])
        d = _as_int(self.val['_depth'])
        return 'core::Array3DView(width=%d, height=%d, depth=%d)' % (w, h, d)

    def display_hint(self):
        return 'array'

    def children(self):
        w = _as_int(self.val['_width'])
        h = _as_int(self.val['_height'])
        d = _as_int(self.val['_depth'])
        data = self.val['_data']
        if _is_null(data) or w <= 0 or h <= 0 or d <= 0:
            return
        shown = 0
        limit = _print_elements()
        for z in range(d):
            for y in range(h):
                for x in range(w):
                    yield '[%d][%d][%d]' % (z, y, x), data[x + w * (y + h * z)]
                    shown += 1
                    if shown >= limit:
                        return


class MemberWrapperPrinter:
    def __init__(self, val, name, fields):
        self.val = _strip(val)
        self.name = name
        self.fields = fields

    def to_string(self):
        return self.name

    def children(self):
        for field in self.fields:
            try:
                yield field, self.val[field]
            except gdb.error as exc:
                yield field, str(exc)


class RegionPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        mins = self.val['_mins']
        maxs = self.val['_maxs']
        return 'voxel::Region(mins=%s, maxs=%s)' % (mins, maxs)

    def children(self):
        yield 'mins', self.val['_mins']
        yield 'maxs', self.val['_maxs']
        yield 'cells', self.val['_width']
        yield 'voxels', self.val['_voxels']
        yield 'center', self.val['_center']


def _map_printer(name, keys_only=False):
    return lambda val: HashMapPrinter(val, name=name, keys_only=keys_only)


def _wrapper(name, *fields):
    return lambda val: MemberWrapperPrinter(val, name, fields)


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("vengi")
    pp.add_printer("glm::vec", r"^glm::vec<", VecPrinter)
    pp.add_printer("glm::mat", r"^glm::mat<", MatPrinter)
    pp.add_printer("core::String", r"^core::String$", StringPrinter)
    pp.add_printer("core::DynamicArray", r"^core::DynamicArray<.*>$", DynamicArrayPrinter)
    pp.add_printer("core::Buffer", r"^core::Buffer<.*>$", BufferPrinter)
    pp.add_printer("core::Array2DView", r"^core::Array2DView<.*>$", Array2DViewPrinter)
    pp.add_printer("core::Array3DView", r"^core::Array3DView<.*>$", Array3DViewPrinter)
    pp.add_printer("core::Array", r"^core::Array<.*>$", ArrayPrinter)
    pp.add_printer("core::Vector", r"^core::Vector<.*>$", VectorPrinter)
    pp.add_printer("core::Stack", r"^core::Stack<.*>$", StackPrinter)
    pp.add_printer("core::DynamicStack", r"^core::DynamicStack<.*>$", DynamicStackPrinter)
    pp.add_printer("core::Queue", r"^core::Queue<.*>$", QueuePrinter)
    pp.add_printer("core::RingBuffer", r"^core::RingBuffer<.*>$", RingBufferPrinter)
    pp.add_printer("core::MRUBuffer", r"^core::MRUBuffer<.*>$", MRUBufferPrinter)
    pp.add_printer("core::BufferView", r"^core::BufferView<.*>$", BufferViewPrinter)
    pp.add_printer("core::List", r"^core::List<.*>$", ListPrinter)
    pp.add_printer("core::DynamicList", r"^core::DynamicList<.*>$", DynamicListPrinter)
    pp.add_printer("core::Map", r"^core::Map<.*>$", _map_printer('core::Map'))
    pp.add_printer("core::DynamicMap", r"^core::DynamicMap<.*>$", _map_printer('core::DynamicMap'))
    pp.add_printer("core::DynamicMultiMap", r"^core::DynamicMultiMap<.*>$", _map_printer('core::DynamicMultiMap'))
    pp.add_printer("core::ParallelMap", r"^core::ParallelMap<.*>$", _map_printer('core::ParallelMap'))
    pp.add_printer("core::DynamicParallelMap", r"^core::DynamicParallelMap<.*>$", _map_printer('core::DynamicParallelMap'))
    pp.add_printer("core::Set", r"^core::Set<.*>$", _map_printer('core::Set', keys_only=True))
    pp.add_printer("core::DynamicSet", r"^core::DynamicSet<.*>$", _map_printer('core::DynamicSet', keys_only=True))
    pp.add_printer("core::SharedPtrSet", r"^core::SharedPtrSet<.*>$", _map_printer('core::SharedPtrSet', keys_only=True))
    pp.add_printer("core::BitSet", r"^core::BitSet<.*>$", BitSetPrinter)
    pp.add_printer("core::DynamicBitSet", r"^core::DynamicBitSet$", DynamicBitSetPrinter)
    pp.add_printer("core::PriorityQueue", r"^core::PriorityQueue<.*>$", _wrapper('core::PriorityQueue', '_data'))
    pp.add_printer("core::ConcurrentQueue", r"^core::ConcurrentQueue<.*>$", _wrapper('core::ConcurrentQueue', '_data', '_abort'))
    pp.add_printer("core::ConcurrentDynamicArray", r"^core::ConcurrentDynamicArray<.*>$", _wrapper('core::ConcurrentDynamicArray', '_data'))
    pp.add_printer("core::ConcurrentPriorityQueue", r"^core::ConcurrentPriorityQueue<.*>$", _wrapper('core::ConcurrentPriorityQueue', '_data', '_abort'))
    pp.add_printer("core::ConcurrentSet", r"^core::ConcurrentSet<.*>$", _wrapper('core::ConcurrentSet', '_data'))
    pp.add_printer("voxel::Region", r"^voxel::Region$", RegionPrinter)
    return pp


def register_vengi_pretty_printers():
    printer = build_pretty_printer()
    try:
        gdb.printing.register_pretty_printer(gdb.current_objfile(), printer, replace=True)
    except TypeError:
        gdb.printing.register_pretty_printer(gdb.current_objfile(), printer)


register_vengi_pretty_printers()
