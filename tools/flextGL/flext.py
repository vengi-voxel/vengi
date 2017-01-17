import time, os
import urllib.request
import os.path
import re

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

if script_dir == '':
    script_dir = '.'

script_dir += '/'
spec_dir = script_dir + 'spec/'
default_template_root = script_dir + 'templates/'


################################################################################
# Name and location for spec file(s)
################################################################################

specFileList = ['gl.xml']
specURL = 'http://www.opengl.org/registry/api/'
gl_xml_file = '%s/gl.xml' % spec_dir


################################################################################
# Spec file download
################################################################################

def file_age(filename):
    return (time.time() - os.path.getmtime(filename)) / 3600.0

def download_spec(always_download = False):
    if not os.path.exists(spec_dir):
        os.makedirs(spec_dir)

    for fileName in specFileList:
        filePath = '%s%s' % (spec_dir, fileName)
        if (always_download or not os.path.exists(filePath) or file_age(filePath) > 3 * 24):
            fileURL  = '%s%s' % (specURL, fileName)
            print ('Downloading %s' % fileURL)
            urllib.request.urlretrieve(fileURL, filePath)


################################################################################
# Profile file parsing
################################################################################

class Version():
    def __init__(self, major, minor, profile_or_api):
        # 'gl', 'gles1' or 'gles2'
        self.api = 'gl' + profile_or_api if profile_or_api in ['es1', 'es2'] else 'gl'
        self.major = int(major)
        self.minor = int(minor)
        # 'core' or 'compatibility'
        self.profile = profile_or_api if self.api == 'gl' else ''

    def __str__(self):
        return 'OpenGL %d.%d %s' % (self.major, self.minor, self.profile) \
            if self.api == 'gl' else 'OpenGL ES %d.%d' % (self.major, self.minor)

    def int_value(self):
        return 10 * self.major + self.minor;

    
def parse_profile(filename):
    comment_pattern = re.compile('\s*#.*$|\s+$')
    version_pattern = re.compile('\s*version\s+(\d)\.(\d)\s*(core|compatibility|es|)\s*$')
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
                if match.group(3) == 'es':
                    version = Version(match.group(1), match.group(2), 'es1' if match.group(1) == '1' else 'es2')
                else:
                    version = Version(match.group(1), match.group(2), match.group(3))
                    
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

    if funcslist:
        #Functions needed by loader code
        funcslist.append("GetIntegerv")
        funcslist.append("GetStringi")        
    
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
    def __init__(self, api, name, definition, dependent):
        self.api        = api
        self.name       = name
        self.definition = definition
        self.dependent  = dependent
        
class Enum:
    def __init__(self, name, value):
        self.name  = name
        self.value = value
        
class Command:
    def __init__(self, rettype, name, params, requiredTypes):
        self.name          = name
        self.params        = params
        self.returntype    = rettype
        self.requiredTypes = requiredTypes
        
def safe_text(text):
    return '' if (text is None) else text

def xml_extract_all_text(node, substitutes):
    fragments = [safe_text(node.text)]
    for item in list(node):
        fragments.append(substitutes[item.tag] if item.tag in substitutes else safe_text(item.text))
        fragments.append(safe_text(item.tail))
    return ''.join(fragments)

def xml_parse_type_name_pair(node):
    name = node.find('name').text.strip()
    type = xml_extract_all_text(node, {'name':''}).strip()
    ptype = node.find('ptype')
    return (name, type, ptype.text.strip() if ptype != None else None)

def extract_names(feature, selector):
    return [element.attrib['name'] for element in feature.findall('./%s[@name]' % selector)]

def parse_int_version(version_str):
    version_pattern = re.compile('(\d)\.(\d)')
    match = version_pattern.match(version_str)
    return int(match.group(1)) * 10 + int(match.group(2))

def parse_xml_enums(root, api):
    enums = {}

    for enum in root.findall("./enums/enum"):
        if ('api' in enum.attrib and enum.attrib['api'] != api): continue
        name  = enum.attrib['name']
        value = "%s%s" % (enum.attrib['value'], enum.attrib['type']) if 'type' in enum.attrib else enum.attrib['value']
        enums[name] = value

    return enums

def parse_xml_types(root, api):
    types = []

    for type in root.findall("./types/type"):
        if ('api' in type.attrib and type.attrib['api'] != api): continue

        name = type.attrib['name'] if 'name'in type.attrib else type.find('./name').text
        definition = xml_extract_all_text(type, {'apientry' : 'APIENTRY'})

        types.append(Type(type.attrib['api'] if 'api' in type.attrib else None, name, definition, type.attrib['requires'] if 'requires' in type.attrib else None))

    # Go through type list and keep only unique names. Because the
    # specializations are at the end, going in reverse will select only the
    # most specialized ones.
    unique_type_names = {}
    unique_types = []
    for type in reversed(types):
        # Skip the entry if the type is already defined with better
        # specialization for this API
        if type.name in unique_type_names and not type.api and unique_type_names[type.name]: continue

        unique_type_names[type.name] = type.api
        unique_types.append(type)

    # Reverse the list again to keep the original order
    return [t for t in reversed(unique_types)]

def parse_xml_commands(root):
    commands = {}

    for cmd in root.findall("./commands/command"):
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

        commands[name] = Command(rettype, name, params, requiredTypes)

    return commands

def parse_xml_features(root, int_version, api, profile):
    subsets = []

    for feature in root.findall("./feature[@api='%s'][@name][@number]" % api):
        if (parse_int_version(feature.attrib['number'])>int_version):
            continue

        featureName = feature.attrib['name']

        typeList    = []
        enumList    = []
        commandList = []

        for actionSet in list(feature):
            if api == 'gl' and 'profile' in actionSet.attrib and actionSet.attrib['profile'] != profile:
                continue

            if actionSet.tag == 'require':
                typeList.extend(extract_names(actionSet, './type'))
                enumList.extend(extract_names(actionSet, './enum'))
                commandList.extend(extract_names(actionSet, './command'))

            if actionSet.tag == 'remove':
                for subset in subsets:
                    subset.types    = [entry for entry in subset.types    if entry not in set(extract_names(actionSet, './type'))]
                    subset.enums    = [entry for entry in subset.enums    if entry not in set(extract_names(actionSet, './enum'))]
                    subset.commands = [entry for entry in subset.commands if entry not in set(extract_names(actionSet, './command'))]

        subsets.append(APISubset(featureName[3:], typeList, enumList, commandList))

    return subsets

def parse_xml_extensions(root, extensions, api, profile):
    removedEnums    = set()
    removedTypes    = set()
    removedCommands = set()

    subsets = []

    for name, _ in extensions:
        extension = root.find("./extensions/extension[@name='GL_%s']" % name)
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
                if require.attrib['api'] != api: continue
                if 'profile' in require.attrib and require.attrib['profile'] != profile: continue

            subsetTypes += extract_names(require, 'type')
            subsetEnums += extract_names(require, 'enum')
            subsetCommands += extract_names(require, 'command')

        subsets.append(APISubset(name, subsetTypes, subsetEnums, subsetCommands))

    return subsets

def generate_passthru(dependencies, types):
    passthru = ''
    for type in types:
        if (type.name in dependencies):
            if passthru: passthru += '\n'
            passthru += type.definition

    return passthru

def generate_enums(subsets, enums):
    enumsDecl = ''
    for subset in subsets:
        if subset.enums != []:
            if enumsDecl: enumsDecl += '\n\n'
            enumsDecl += '/* GL_%s */\n' % subset.name
            for enumName in subset.enums:
                enumsDecl += '\n#define %s %s' % (enumName, enums[enumName])

    return enumsDecl

def generate_functions(subsets, commands, funcslist, funcsblacklist):
    functions = []
    function_set = set()
    
    for subset in subsets:
        #remove 'gl' suffixes and strip away commands that are already in the list
        subset_functions = []
        for name in subset.commands:
            if name in function_set: continue
            if funcslist and name[2:] not in funcslist: continue
            if funcsblacklist and name[2:] in funcsblacklist: continue
            subset_functions.append(Function(commands[name].returntype, commands[name].name[2:], commands[name].params))
            function_set.add(name)

        functions.append((subset.name, subset_functions))

    return functions

def resolve_type_dependencies(subsets, types, commands):
    requiredTypes = set()

    for subset in subsets:
        requiredTypes |= set(subset.types)
        for cmd in subset.commands:
            requiredTypes |= commands[cmd].requiredTypes

    for type in types:
        if type.name in requiredTypes and type.dependent:
            requiredTypes.add(type.dependent)

    return requiredTypes

def parse_xml(version, extensions, funcslist, funcsblacklist):
    tree = etree.parse(gl_xml_file)
    root = tree.getroot()

    types    = parse_xml_types(root, version.api)
    raw_enums    = parse_xml_enums(root, version.api)
    commands = parse_xml_commands(root)

    subsetsGL  = parse_xml_features  (root, version.int_value(), version.api, version.profile)
    subsetsEXT = parse_xml_extensions(root, extensions, version.api, version.profile)

    subsets  = subsetsGL
    subsets += subsetsEXT

    requiredTypes = resolve_type_dependencies(subsets, types, commands)

    passthru     = generate_passthru(requiredTypes, types)
    enums        = generate_enums(subsets, raw_enums)
    functions    = generate_functions(subsets, commands, funcslist, funcsblacklist)

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

    
