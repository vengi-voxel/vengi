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


import gdb.printing

def _vec_info(v):
    # vec contains either a union of structs or a struct of unions, depending on
    # configuration. gdb can't properly access the named members, and in some
    # cases the names are wrong.
    # It would be simple to cast to an array, similarly to how operator[] is
    # implemented, but values returned by functions called from gdb don't have
    # an address.
    # Instead, recursively find all fields of required type and sort by offset.

    if v.type.code == gdb.TYPE_CODE_REF:
      v = v.referenced_value()

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

    length = v.type.sizeof // T.sizeof
    items = {}
    def find(v, bitpos):
        t = v.type.strip_typedefs()
        if t.code in (gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION):
            for f in t.fields():
                if hasattr(f, "bitpos"): # not static
                    find(v[f], bitpos + f.bitpos)
        elif t == T:
            items[bitpos] = v
    find(v, 0)
    assert len(items) == length
    items = [str(f) for k, f in sorted(items.items())]
    return type_prefix, length, items

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
        V = self.v["value"]
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
        return self.val['_data']['_str']

    # def children(self):
    #     size = int(self.val['_data']['_size'])
    #     buffer = self.val['_data']['_str']
    #     for i in range(size):
    #         yield f'[{i}]', buffer[i]

class DynamicArrayPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        size = self.val['_size']
        capacity = self.val['_capacity']
        return f'core::DynamicArray(size={size}, capacity={capacity})'

    def children(self):
        size = int(self.val['_size'])
        buffer = self.val['_buffer']
        for i in range(size):
            yield f'[{i}]', buffer[i]

class RegionPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        mins = self.val['_mins']
        maxs = self.val['_maxs']
        return f'voxel::Region(mins={mins}, maxs={maxs})'

    def children(self):
        mins = self.val['_mins']
        maxs = self.val['_maxs']
        cells = self.val['_width']
        voxels = self.val['_voxels']
        center = self.val['_center']
        yield 'mins', mins
        yield 'maxs', maxs
        yield 'cells', cells
        yield 'voxels', voxels
        yield 'center', center

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("vengi")
    pp.add_printer("glm::vec", r"^glm::vec<", VecPrinter)
    pp.add_printer("glm::vec", r"^glm::mat<", MatPrinter)
    pp.add_printer("core::String", r"^core::String$", StringPrinter)
    pp.add_printer("core::DynamicArray", "^core::DynamicArray<.*>$", DynamicArrayPrinter)
    pp.add_printer("voxel::Region", "^voxel::Region$", RegionPrinter)
    return pp

gdb.printing.register_pretty_printer(gdb.current_objfile(),
                                     build_pretty_printer())
