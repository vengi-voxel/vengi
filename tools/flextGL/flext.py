import time, os
import urllib.request
import os.path
import re
import sys

# In order to have dicts iterable in insertion order in Python < 3.6
# (https://stackoverflow.com/a/39537308)
if not sys.version_info >= (3, 6, 0):
    from collections import OrderedDict

from glob     import glob

import xml.etree.ElementTree as etree

from wheezy.template.engine   import Engine
from wheezy.template.ext.core import CoreExtension
from wheezy.template.ext.code import CodeExtension
from wheezy.template.loader   import FileLoader

################################################################################
# Find the local directory for this file
################################################################################

script_dir = os.path.dirname(__file__)
spec_dir = os.path.join(script_dir, 'spec')
default_template_root = os.path.join(script_dir, 'templates')


################################################################################
# Name and location for spec file(s)
################################################################################

gl_spec_url = 'http://www.opengl.org/registry/api/gl.xml'

# This URL structure is valid from 1.1.72, older had it differently (and the
# tags were also named differently). I hope this will not be changing much in
# the future, as that would break loading of older versions.
vk_spec_url = 'https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/{}/xml/vk.xml'

################################################################################
# Spec file download
################################################################################

def file_age(filename):
    return (time.time() - os.path.getmtime(filename)) / 3600.0

def download_spec(spec_url, save_as, always_download = False):
    if not os.path.exists(spec_dir):
        os.makedirs(spec_dir)

    spec_file = os.path.join(spec_dir, save_as)

    if always_download or not os.path.exists(spec_file) or file_age(spec_file) > 3*24:
        print ('Downloading %s' % spec_url)
        urllib.request.urlretrieve(spec_url, spec_file)

################################################################################
# Profile file parsing
################################################################################

class Version():
    def __init__(self, major, minor, release, profile_or_api):
        # 'vulkan', 'gl', 'gles1' or 'gles2'
        if profile_or_api == 'vulkan':
            self.api = profile_or_api
        elif profile_or_api in ['es1', 'es2']:
            self.api = 'gl' + profile_or_api
        else:
            self.api = 'gl'
        # Macro name prefix
        if profile_or_api == 'vulkan':
            self.prefix = 'VK_'
        else:
            self.prefix = 'GL_'
        self.major = int(major)
        self.minor = int(minor)
        # Release is for Vulkan only
        self.release = int(release) if release is not None else None
        # 'core' or 'compatibility'
        self.profile = profile_or_api if self.api == 'gl' else ''

    def __str__(self):
        if self.api == 'vulkan':
            return 'Vulkan %d.%d' % (self.major, self.minor)
        elif self.api == 'gl':
            return 'OpenGL %d.%d %s' % (self.major, self.minor, self.profile)
        else:
            return 'OpenGL ES %d.%d' % (self.major, self.minor)

    def int_value(self):
        return 10 * self.major + self.minor;


def parse_profile(filename):
    comment_pattern = re.compile('\s*#.*$|\s+$')
    version_pattern = re.compile('\s*version\s+(\d)\.(\d)(\.(\d+))?\s*(core|compatibility|es|vulkan|)\s*$')
    extension_pattern = re.compile('\s*extension\s+(\w+)\s+(required|optional)\s*$')
    functions_pattern = re.compile('\s*(begin|end) functions\s+(blacklist)?$')
    function_pattern = re.compile('\s*[A-Z][A-Za-z0-9]+$')

    version = None
    extensions = []
    extension_set = set()
    funcslist = []
    funcsblacklist = []

    function_mode  = False # Are we in function mode?
    blacklist_mode = False # While in function mode, are we blacklisting?

    with open(filename, 'r') as file:
        for line_no,line in enumerate(file, start=1):

            # Comment: ignore line
            match = comment_pattern.match(line)
            if match:
                continue

            # Begin/End Function list mode
            match = functions_pattern.match(line)
            if match:
                if function_mode == False and match.group(1) == 'begin':
                    function_mode  = True
                    blacklist_mode = (match.group(2) == 'blacklist')
                    continue
                elif function_mode == True and match.group(1) == 'end' and blacklist_mode == (match.group(2) == 'blacklist'):
                    function_mode = False
                    continue
                else:
                    print ('Mismatched \'begin/end function\' (%s:%d): %s' % (filename, line_no, line))
                    exit(1)

            # Parse functions if in function list mode
            if function_mode:
                for name in line.split():
                    if not function_pattern.match(name):
                        print ('\'%s\' does not appear to be a valid OpenGL function name (%s:%d): %s' % (name, filename, line_no, line))
                        exit(1)
                    if blacklist_mode:
                        funcsblacklist.append(name)
                    else:
                        funcslist.append(name)
                continue

            # Version command
            match = version_pattern.match(line)
            if match:
                if version != None:
                    print ('Error (%s:%d): Duplicate version statement' % (filename,line_no))
                    exit(1)
                if match.group(5) == 'es':
                    version = Version(match.group(1), match.group(2), None, 'es1' if match.group(1) == '1' else 'es2')
                else:
                    version = Version(match.group(1), match.group(2), match.group(4), match.group(5))

                continue

            # Extension command
            match = extension_pattern.match(line)
            if match:
                if match.group(1) in extension_set:
                    print ('Error (%s:%d): Duplicate extension statement' % (filename, line_no))
                    exit(1)

                extension_set.add(match.group(1))
                extensions.append((match.group(1), match.group(2) == 'required'))
                continue

            # Unknown command: Error
            print ('Syntax Error (%s:%d): %s' % (filename, line_no, line))
            exit(1)

    if not version:
        print('Error ({}): Missing version statement'.format(filename))
        exit(1)

    if funcslist:
        #Functions needed by loader code
        if version.api == 'vulkan':
            funcslist += ['GetInstanceProcAddr', 'GetDeviceProcAddr']
        else:
            funcslist += ['GetIntegerv', 'GetStringi']

    return version, extensions, set(funcslist), set(funcsblacklist)


################################################################################
# XML spec file parsing
################################################################################

class Function:
    def __init__(self, rettype, name, params):
        self.name          = name
        self.params        = params
        self.returntype    = rettype

    def param_list_string(self):
        return 'void' if len(self.params) == 0 else ', '.join(['%s %s' % (t, p) for p,t in self.params])

    def param_type_list_string(self):
        return 'void' if len(self.params) == 0 else ', '.join(['%s' % t for p,t in self.params])

class APISubset:
    def __init__(self, name, types, enums, commands):
        self.name     = name
        self.types    = types
        self.enums    = enums
        self.commands = commands

class Type:
    def __init__(self, api, name, definition, is_bitmask, required_types, required_enums, struct_extends, alias):
        self.api        = api
        self.name       = name
        self.definition = definition
        self.is_dependent = bool(struct_extends)
        self.is_bitmask = is_bitmask
        self.required_types = required_types
        self.required_enums = required_enums
        self.struct_extends = struct_extends
        self.alias      = alias

class Command:
    def __init__(self, rettype, name, params, requiredTypes):
        self.name          = name
        self.params        = params
        self.returntype    = rettype
        self.requiredTypes = requiredTypes

def xml_extract_all_text(node, substitutes):
    fragments = []
    if node.text: fragments += [node.text]
    for item in list(node):
        if item.tag in substitutes:
            fragments += [substitutes[item.tag]]
        elif item.text:
            # Sometimes <type> and <name> is not separated with a space in
            # vk.xml, add it here
            if fragments and fragments[-1] and fragments[-1][-1].isalnum():
                if fragments[-1][-1].isalnum() and item.text and item.text[0].isalnum():
                    fragments += [' ']
            fragments += [item.text]
        if item.tail: fragments.append(item.tail)
    return ''.join(fragments).strip()

def xml_parse_type_name_pair(node):
    name = node.find('name').text.strip()
    type = xml_extract_all_text(node, {'name':''})
    # gl.xml has <ptype> while vk.xml has <type>
    ptype = node.find('ptype')
    if ptype == None: ptype = node.find('type')
    return (name, type, ptype.text.strip() if ptype != None else None)

def extract_enums(feature, enum_extensions, extension_number = None):
    subsetEnums = []

    for enum in feature.findall('enum'):
        enum_name = enum.attrib['name']

        if 'extends' in enum.attrib:
            extends = enum.attrib['extends']

            # VkSamplerAddressMode from VK_KHR_sampler_mirror_clamp_to_edge
            # has an explicit value. The sanest way, yet they say "this
            # is a special case, and should not be repeated".
            if 'value' in enum.attrib:
                value = enum.attrib['value']

            # Bit position
            elif 'bitpos' in enum.attrib:
                value = '1 << {}'.format(enum.attrib['bitpos'])

            # Alias
            elif 'alias' in enum.attrib:
                value = enum.attrib['alias']

            # Calculate enum value from an overengineered set of
            # inputs. See the spec for details:
            # https://www.khronos.org/registry/vulkan/specs/1.1/styleguide.html#_assigning_extension_token_values
            else:
                base_value = 1000000000
                range_size = 1000
                if 'extnumber' in enum.attrib:
                    number = int(enum.attrib['extnumber'])
                else:
                    assert extension_number
                    number = extension_number
                value = base_value + (int(number) - 1)*range_size + int(enum.attrib['offset'])
                if enum.attrib.get('dir') == '-': value *= -1
                value = str(value)

            if extends not in enum_extensions: enum_extensions[extends] = []
            enum_extensions[extends] += [(enum_name, value)]

        else:
            # Vulkan enums can provide the value directly next to
            # referencing some external enum value
            if 'value' in enum.attrib:
                subsetEnums += [(enum_name, enum.attrib['value'])]
            else:
                subsetEnums += [(enum_name, None)]

    return subsetEnums, enum_extensions

def extract_names(feature, selector):
    return [element.attrib['name'] for element in feature.findall('./%s[@name]' % selector)]

def parse_int_version(version_str):
    version_pattern = re.compile('(\d)\.(\d)')
    match = version_pattern.match(version_str)
    return int(match.group(1)) * 10 + int(match.group(2))

def parse_xml_enums(root, api):
    # In order to have dicts iterable in insertion order in Python < 3.6
    # (https://stackoverflow.com/a/39537308)
    if sys.version_info >= (3, 6, 0):
        enums = {}
    else:
        enums = OrderedDict()

    for enum in root.findall("./enums/enum"):
        if ('api' in enum.attrib and enum.attrib['api'] != api): continue
        name  = enum.attrib['name']
        if 'type' in enum.attrib:
            value = "%s%s" % (enum.attrib['value'], enum.attrib['type'])
        elif 'bitpos' in enum.attrib:
            value = "1 << {}".format(enum.attrib['bitpos'])
        # GL defines both value and alias, prefer values because the original
        # aliased value might not exist
        elif 'value' in enum.attrib:
            value = enum.attrib['value']
        else:
            value = enum.attrib['alias']
            assert value in enums
        enums[name] = value

    return enums

def parse_xml_types(root, enum_extensions, promoted_enum_extensions, api):
    types = []

    for type in root.findall("./types/type"):
        if ('api' in type.attrib and type.attrib['api'] != api): continue

        name = type.attrib['name'] if 'name'in type.attrib else type.find('./name').text

        # Type dependencies
        dependencies = set()
        enum_dependencies = set()
        if 'requires' in type.attrib:
            dependencies |= set([type.attrib['requires']])

        # Vulkan type aliases
        alias = None
        if 'alias' in type.attrib:
            dependencies.add(type.attrib['alias']);
            definition = '\ntypedef {} {};'.format(type.attrib['alias'], type.attrib['name'])
            alias = type.attrib['alias']

        # Struct / union definition in Vulkan
        elif 'category' in type.attrib and type.attrib['category'] in ['struct', 'union']:
            members = []
            for m in type.findall('member'):
                members += ['    {};'.format(xml_extract_all_text(m, {'comment': ''}))]

                # Add all member <type>s and array sizes (<enum>) to
                # dependencies. Some structures such as VkBaseInStructure or
                # VkBaseOutStructure have a pointer to itself, which would be
                # a cycle -- avoid that
                dependencies |= set([t.text for t in m.findall('type')]) - set([name])
                enum_dependencies |= set([t.text for t in m.findall('enum')])

            # Due to VkBaseInStructure / VkBaseOutStructure members pointing to
            # itself, there needs to be `typedef struct Name { ... } Name;`
            # instead of just `typedef struct Name { ... } Name;`.
            definition = '\ntypedef {0} {2} {{\n{1}\n}} {2};'.format(type.attrib['category'], '\n'.join(members), type.attrib['name'])

        # Enum definition in Vulkan
        elif 'category' in type.attrib and type.attrib['category'] == 'enum':
            values = []
            name = type.attrib['name']
            enumdef = root.find("./enums[@name='{}']".format(name))
            if enumdef: # Some Vulkan enums are empty (bitsets)
                for enum in enumdef.findall('enum'):
                    if 'bitpos' in enum.attrib:
                        values += ['    {} = 1 << {}'.format(enum.attrib['name'], enum.attrib['bitpos'])]
                    elif 'alias' in enum.attrib:
                        values += ['    {} = {}'.format(enum.attrib['name'], enum.attrib['alias'])]
                    else:
                        values += ['    {} = {}'.format(enum.attrib['name'], enum.attrib['value'])]
                if name in enum_extensions:

                    # Extension enum values might be promoted to core in later
                    # versions, create a map with their values to avoid having
                    # aliases to nonexistent values
                    extensions = {}
                    if name in promoted_enum_extensions:
                        for value, number in promoted_enum_extensions[name]:
                            extensions[value] = number

                    # Value is either a concrete value, an existing alias or
                    # an extracted value from a promoted alias above
                    for extension, value in enum_extensions[name]:
                        values += ['    {} = {}'.format(extension, extensions[value] if value in extensions else value)]

                definition = '\ntypedef enum {{\n{}\n}} {};'.format(',\n'.join(values), name)

            else: # ISO C++ forbids empty unnamed enum, work around that
                definition = '\ntypedef int {};'.format(name)

        # Classic type definition
        else:
            definition = xml_extract_all_text(type, {'apientry' : 'APIENTRY'})

            # Add all member types to dependencies (can be more than one in
            # case of function pointer definitions)
            dependencies |= set([t.text for t in type.findall('type')])

        # If the type defines something but the definition is empty, our
        # parsing is broken. OTOH, there can be proxy types such as
        # <type requires="X11/Xlib.h" name="Display"/> that don't define
        # anything.
        assert not type or definition.strip()

        types.append(Type(type.attrib['api'] if 'api' in type.attrib else None, name, definition, type.attrib.get('category') == 'bitmask', dependencies, enum_dependencies, type.attrib['structextends'].split(',') if 'structextends' in type.attrib else [], alias))

    # Go through type list and keep only unique names. Because the
    # specializations are at the end, going in reverse will select only the
    # most specialized ones.
    unique_type_names = {}
    unique_types = []
    for type in reversed(types):
        # Skip the entry if the type is already defined with better
        # specialization for this API
        if type.name in unique_type_names and not type.api and unique_type_names[type.name].api: continue

        unique_type_names[type.name] = type
        unique_types.append(type)

    # Mark all type dependencies as such. In the end only types that are
    # (indirectly) referenced by a command and top-level types (that are not
    # referenced by any command, such as indirect draw structures) will get
    # written to the output.
    for type in unique_types:
        for dependency in type.required_types:
            unique_type_names[dependency].is_dependent = True

    # Resolve dependency information for aliases the same as their sources
    for type in unique_types:
        if not type.alias: continue
        type.is_dependent = unique_type_names[type.alias].is_dependent
        type.struct_extends = unique_type_names[type.alias].struct_extends

    # Bubble up recursive type dependencies. A type dependency can also have a
    # transitive enum dependency, but that's fortunately not recursive, so we
    # recurse only for types.
    def gather_type_dependencies(type):
        dependencies = set()
        enum_dependencies = set()
        for d in type.required_types:
            dependencies.add(d)
            new_dependencies, new_enum_dependencies = gather_type_dependencies(unique_type_names[d])
            dependencies |= new_dependencies
            enum_dependencies |= new_enum_dependencies
        for d in type.required_enums:
            enum_dependencies.add(d)
        return dependencies, enum_dependencies
    for type in unique_types:
        type.required_types, type.required_enums = gather_type_dependencies(type)

    # Reverse the list again to keep the original order
    return [t for t in reversed(unique_types)], unique_type_names

def parse_xml_commands(root, type_map):
    commands = {}

    for cmd in root.findall("./commands/command"):
        if 'alias' in cmd.attrib:
            # Assuming the alias is always defined *after* the command it
            # aliases
            name = cmd.attrib['name']
            aliased = commands[cmd.attrib['alias']]
            commands[name] = Command(aliased.returntype, name, aliased.params, aliased.requiredTypes)
            continue

        requiredTypes = set()
        name, rettype, requiredType = xml_parse_type_name_pair(cmd.find('./proto'))
        if requiredType!=None:
            requiredTypes.add(requiredType)

        params = []
        for item in cmd.findall("./param"):
            pname, ptype, requiredType = xml_parse_type_name_pair(item)
            params.append((pname, ptype))

            if requiredType!=None:
                requiredTypes.add(requiredType)

        # Mark all types required by a command as dependent
        for type in requiredTypes: type_map[type].is_dependent = True

        commands[name] = Command(rettype, name, params, requiredTypes)

    return commands

def parse_xml_features(root, version):
    enum_extensions = {}
    promoted_enum_extensions = {}
    subsets = []

    for feature in root.findall("./feature[@api='%s'][@name][@number]" % version.api):
        # Some Vulkan extension enums get promoted to core in later versions
        # and we can't ignore them because the extension enums would then alias
        # to nonexistent values
        if (parse_int_version(feature.attrib['number'])>version.int_value()):
            for actionSet in list(feature):
                _, promoted_enum_extensions = extract_enums(actionSet, promoted_enum_extensions)
            continue

        featureName = feature.attrib['name']

        typeList    = []
        enumList    = []
        commandList = []

        for actionSet in list(feature):
            if version.api == 'gl' and 'profile' in actionSet.attrib and actionSet.attrib['profile'] != version.profile:
                continue

            if actionSet.tag == 'require':
                typeList.extend(extract_names(actionSet, './type'))
                commandList.extend(extract_names(actionSet, './command'))

                enums_to_add, enum_extensions = extract_enums(actionSet, enum_extensions)
                enumList += enums_to_add

            if actionSet.tag == 'remove':
                for subset in subsets:
                    subset.types    = [entry for entry in subset.types    if entry not in set(extract_names(actionSet, './type'))]
                    subset.enums    = [(entry, entryValue) for entry, entryValue in subset.enums    if entry not in set(extract_names(actionSet, './enum'))]
                    subset.commands = [entry for entry in subset.commands if entry not in set(extract_names(actionSet, './command'))]

        subsets.append(APISubset(featureName[3:], typeList, enumList, commandList))

    return subsets, enum_extensions, promoted_enum_extensions

def parse_xml_extensions(root, extensions, enum_extensions, version):
    subsets = []

    for name, _ in extensions:
        extension = root.find("./extensions/extension[@name='{}{}']".format(version.prefix, name))
        if (extension==None):
            print ('%s is not an extension' % name)
            continue

        subsetTypes = []
        subsetEnums = []
        subsetCommands = []

        for require in extension.findall('./require'):
            # Given set of names is restricted to some API or profile subset
            # (e.g. KHR_debug has different set of names for 'gl' and 'gles2')
            if 'api' in require.attrib:
                if require.attrib['api'] != version.api: continue
                if 'profile' in require.attrib and require.attrib['profile'] != version.profile: continue

            subsetTypes += extract_names(require, 'type')
            subsetCommands += extract_names(require, 'command')

            # The 'number' attribute is available only in vk.xml, extract_enums()
            # asserts that it's available if needed
            enums_to_add, enum_extensions = extract_enums(require, enum_extensions, extension.attrib.get('number'))
            subsetEnums += enums_to_add

        subsets.append(APISubset(name, subsetTypes, subsetEnums, subsetCommands))

    return subsets, enum_extensions

def generate_passthru(dependencies, types):
    written_types = set()

    def write_type(type, written_types, passthru):
        # We're handling vk_platform ourselves; VK_API_VERSION is deprecated
        if not type.definition or type.name in ['vk_platform', 'VK_API_VERSION']:
            return passthru

        # Ensure all dependencies are written already. Using a simple linear
        # search because this occurs for only like three or four types in the
        # whole vk.xml, so I don't really care about performance. Vulkan
        # bitmask types have enum dependencies, but that's not enforced by the
        # type system so we don't need to handle that. Explicitly sorting the
        # dependencies to get a deterministic order.
        if not type.is_bitmask:
            for dependency in sorted(type.required_types):
                if dependency in written_types: continue
                for t in types:
                    if t.name == dependency:
                        passthru = write_type(t, written_types, passthru)
                        break
                else: assert False # pragma: no cover

        if passthru: passthru += '\n'
        passthru += type.definition
        written_types.add(type.name)
        return passthru

    passthru = ''
    for type in types:
        if type.name in dependencies and type.name not in written_types:
            passthru = write_type(type, written_types, passthru)

    return passthru

def generate_enums(subsets, requiredEnums, enums, version):
    # Vulkan enum values can alias each other, ensure that the alias source is
    # pulled in as well. Assume there's no recursive dependency.
    for subset in subsets:
        for enumName, _ in subset.enums:
            if enumName in enums and enums[enumName] in enums:
                requiredEnums.add(enums[enumName])
                assert enums[enums[enumName]] not in enums

    enumsDecl = ''

    # Vulkan API constants that are required by type definitions but not part
    # of any subset. Iterating over the enums dict instead of iterating over
    # the requiredEnums set in order to retain the XML order and have
    # deterministic output. In Python 3.6+ dict preserves insertion order for
    # iterations, in older versions we use collections.OrderedDict.
    for name, definition in enums.items():
        if name in requiredEnums:
            if enumsDecl: enumsDecl += '\n'
            enumsDecl += '#define {} {}'.format(name, definition)

    for subset in subsets:
        if subset.enums != []:
            if enumsDecl: enumsDecl += '\n\n'
            enumsDecl += '/* {}{} */\n'.format(version.prefix, subset.name)
            for enumName, enumValue in subset.enums:
                # Vulkan enums can provide the value directly next to
                # referencing some external enum value
                if enumValue:
                    enumsDecl += '\n#define {} {}'.format(enumName, enumValue)
                else:
                    enumsDecl += '\n#define {} {}'.format(enumName, enums[enumName])

    return enumsDecl

def generate_functions(subsets, commands, funcslist, funcsblacklist):
    functions = []
    function_set = set()
    required_types = set()

    for subset in subsets:
        # remove gl/vk prefixes and strip away commands that are already in the
        # list
        subset_functions = []
        for name in subset.commands:
            if name in function_set: continue
            if funcslist and name[2:] not in funcslist: continue
            if funcsblacklist and name[2:] in funcsblacklist: continue
            subset_functions.append(Function(commands[name].returntype, commands[name].name[2:], commands[name].params))

            function_set.add(name)
            required_types |= commands[name].requiredTypes

        functions.append((subset.name, subset_functions))

    return functions, required_types

def resolve_type_dependencies(subsets, requiredTypes, types):
    requiredEnums = set()

    types_from_subsets = set()
    for subset in subsets:
        types_from_subsets |= set(subset.types)

    for type in types:
        # If given type is a top-level one required by one of the subsets
        # (i.e., not referenced (indirectly) by any command such as indirect
        # draw structures), include it as well.
        if type.name in types_from_subsets and not type.is_dependent:
            requiredTypes.add(type.name)

        # If given type is required by one of the subsets and is an alias to a
        # type that's required, include it too.
        if type.name in types_from_subsets and type.alias in requiredTypes:
            requiredTypes.add(type.name)

    # If given type is required, add also all its dependencies to required
    # types.
    for type in types:
        if type.name in requiredTypes:
            requiredTypes |= type.required_types
            requiredEnums |= type.required_enums

    # If there are types that extend required types, add them as well. This is
    # done after everything else, so extensions that are not to top-level types
    # are included as well.
    for type in types:
        if type.name in types_from_subsets:
            for base in type.struct_extends:
                if base in requiredTypes:
                    requiredTypes.add(type.name)
                    requiredTypes |= type.required_types
                    requiredEnums |= type.required_enums

    return requiredTypes, requiredEnums

def parse_xml(file, version, extensions, funcslist, funcsblacklist):
    tree = etree.parse(os.path.join(spec_dir, file))
    root = tree.getroot()

    subsets, enum_extensions, promoted_enum_extensions = parse_xml_features(root, version)
    subset_extensions, extension_enum_extensions = parse_xml_extensions(root, extensions, enum_extensions, version)
    subsets += subset_extensions
    enum_extensions.update(extension_enum_extensions)

    types, type_map = parse_xml_types(root, enum_extensions, promoted_enum_extensions, version.api)
    raw_enums = parse_xml_enums(root, version.api)
    commands = parse_xml_commands(root, type_map)

    functions, requiredTypes = generate_functions(subsets, commands, funcslist, funcsblacklist)
    requiredTypes, requiredEnums = resolve_type_dependencies(subsets, requiredTypes, types)
    passthru = generate_passthru(requiredTypes, types)
    enums = generate_enums(subsets, requiredEnums, raw_enums, version)

    return passthru, enums, functions, types, raw_enums


################################################################################
# Source generation
################################################################################

def generate_source(options, version, enums, functions_by_category, passthru, extensions, types, raw_enums):
    template_pattern = re.compile("(.*).template")

    # Sort by categories and sort the functions inside the categories
    functions_by_category = sorted(functions_by_category
                                  ,key=lambda x: x[0])
    functions_by_category = list(map(lambda c: (c[0], sorted(c[1], key=lambda x: x.name))
                                ,functions_by_category))

    template_namespace = {'passthru'  : passthru,
                          'functions' : functions_by_category,
                          'enums'     : enums,
                          'options'   : options,
                          'version'   : version,
                          'extensions': extensions,
                          'types': types,
                          'raw_enums': raw_enums}
    if not os.path.isdir(options.template_dir):
        print ('%s is not a directory' % options.template_dir)
        exit(1)

    if os.path.exists(options.outdir) and not os.path.isdir(options.outdir):
        print ('%s is not a directory' % options.outdir)
        exit(1)

    if not os.path.exists(options.outdir):
        os.mkdir(options.outdir)

    engine = Engine(loader=FileLoader([options.template_dir]),extensions=[CoreExtension(),CodeExtension()])

    generatedFiles = 0
    allFiles       = 0;

    for template_path in glob('%s/*.template' % os.path.abspath(options.template_dir)):

        infile = os.path.basename(template_path)
        outfile = '%s/%s' % (options.outdir, template_pattern.match(infile).group(1))

        template = engine.get_template(infile)

        allFiles += 1

        with open(outfile, 'w') as out:
            out.write(template.render(template_namespace))
            print("Successfully generated %s" % outfile)
            generatedFiles += 1;

    print("Generated %d of %d files" % (generatedFiles, allFiles))
