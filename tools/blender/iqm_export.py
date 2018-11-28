# This script is licensed as public domain.

bl_info = {
    "name": "Export Inter-Quake Model (.iqm/.iqe)",
    "author": "Lee Salzman",
    "version": (2016, 2, 9),
    "blender": (2, 74, 0),
    "location": "File > Export > Inter-Quake Model",
    "description": "Export to the Inter-Quake Model format (.iqm/.iqe)",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Import-Export"}

import os, struct, math
import mathutils
import bpy
import bpy_extras.io_utils

IQM_POSITION     = 0
IQM_TEXCOORD     = 1
IQM_NORMAL       = 2
IQM_TANGENT      = 3
IQM_BLENDINDEXES = 4
IQM_BLENDWEIGHTS = 5
IQM_COLOR        = 6
IQM_CUSTOM       = 0x10

IQM_BYTE   = 0
IQM_UBYTE  = 1
IQM_SHORT  = 2
IQM_USHORT = 3
IQM_INT    = 4
IQM_UINT   = 5
IQM_HALF   = 6
IQM_FLOAT  = 7
IQM_DOUBLE = 8

IQM_LOOP = 1

IQM_HEADER      = struct.Struct('<16s27I')
IQM_MESH        = struct.Struct('<6I')
IQM_TRIANGLE    = struct.Struct('<3I')
IQM_JOINT       = struct.Struct('<Ii10f')
IQM_POSE        = struct.Struct('<iI20f')
IQM_ANIMATION   = struct.Struct('<3IfI')
IQM_VERTEXARRAY = struct.Struct('<5I')
IQM_BOUNDS      = struct.Struct('<8f')

MAXVCACHE = 32

class Vertex:
    def __init__(self, index, coord, normal, uv, weights, color):
        self.index   = index
        self.coord   = coord
        self.normal  = normal
        self.uv      = uv
        self.weights = weights
        self.color   = color

    def normalizeWeights(self):
        # renormalizes all weights such that they add up to 255
        # the list is chopped/padded to exactly 4 weights if necessary
        if not self.weights:
            self.weights = [ (0, 0), (0, 0), (0, 0), (0, 0) ]
            return
        self.weights.sort(key = lambda weight: weight[0], reverse=True)
        if len(self.weights) > 4:
            del self.weights[4:]
        totalweight = sum([ weight for (weight, bone) in self.weights])
        if totalweight > 0:
            self.weights = [ (int(round(weight * 255.0 / totalweight)), bone) for (weight, bone) in self.weights]
            while len(self.weights) > 1 and self.weights[-1][0] <= 0:
                self.weights.pop()
        else:
            totalweight = len(self.weights)
            self.weights = [ (int(round(255.0 / totalweight)), bone) for (weight, bone) in self.weights]
        totalweight = sum([ weight for (weight, bone) in self.weights])
        while totalweight != 255:
            for i, (weight, bone) in enumerate(self.weights):
                if totalweight > 255 and weight > 0:
                    self.weights[i] = (weight - 1, bone)
                    totalweight -= 1
                elif totalweight < 255 and weight < 255:
                    self.weights[i] = (weight + 1, bone)
                    totalweight += 1
        while len(self.weights) < 4:
            self.weights.append((0, self.weights[-1][1]))

    def calcScore(self):
        if self.uses:
            self.score = 2.0 * pow(len(self.uses), -0.5)
            if self.cacherank >= 3:
                self.score += pow(1.0 - float(self.cacherank - 3)/MAXVCACHE, 1.5)
            elif self.cacherank >= 0:
                self.score += 0.75
        else:
            self.score = -1.0

    def neighborKey(self, other):
        if self.coord < other.coord:
            return (self.coord.x, self.coord.y, self.coord.z, other.coord.x, other.coord.y, other.coord.z, tuple(self.weights), tuple(other.weights))
        else:
            return (other.coord.x, other.coord.y, other.coord.z, self.coord.x, self.coord.y, self.coord.z, tuple(other.weights), tuple(self.weights))

    def __hash__(self):
        return self.index

    def __eq__(self, v):
        return self.coord == v.coord and self.normal == v.normal and self.uv == v.uv and self.weights == v.weights and self.color == v.color


class Mesh:
    def __init__(self, name, material, verts):
        self.name      = name
        self.material  = material
        self.verts     = [ None for v in verts ]
        self.vertmap   = {}
        self.tris      = []

    def calcTangents(self):
        # See "Tangent Space Calculation" at http://www.terathon.com/code/tangent.html
        for v in self.verts:
            v.tangent = mathutils.Vector((0.0, 0.0, 0.0))
            v.bitangent = mathutils.Vector((0.0, 0.0, 0.0))
        for (v0, v1, v2) in self.tris:
            dco1 = v1.coord - v0.coord
            dco2 = v2.coord - v0.coord
            duv1 = v1.uv - v0.uv
            duv2 = v2.uv - v0.uv
            tangent = dco2*duv1.y - dco1*duv2.y
            bitangent = dco2*duv1.x - dco1*duv2.x
            if dco2.cross(dco1).dot(bitangent.cross(tangent)) < 0:
                tangent.negate()
                bitangent.negate()
            v0.tangent += tangent
            v1.tangent += tangent
            v2.tangent += tangent
            v0.bitangent += bitangent
            v1.bitangent += bitangent
            v2.bitangent += bitangent
        for v in self.verts:
            v.tangent = v.tangent - v.normal*v.tangent.dot(v.normal)
            v.tangent.normalize()
            if v.normal.cross(v.tangent).dot(v.bitangent) < 0:
                v.bitangent = -1.0
            else:
                v.bitangent = 1.0

    def optimize(self):
        # Linear-speed vertex cache optimization algorithm by Tom Forsyth
        for v in self.verts:
            if v:
                v.index = -1
                v.uses = []
                v.cacherank = -1
        for i, (v0, v1, v2) in enumerate(self.tris):
            v0.uses.append(i)
            v1.uses.append(i)
            v2.uses.append(i)
        for v in self.verts:
            if v:
                v.calcScore()

        besttri = -1
        bestscore = -42.0
        scores = []
        for i, (v0, v1, v2) in enumerate(self.tris):
            scores.append(v0.score + v1.score + v2.score)
            if scores[i] > bestscore:
                besttri = i
                bestscore = scores[i]

        vertloads = 0 # debug info
        vertschedule = []
        trischedule = []
        vcache = []
        while besttri >= 0:
            tri = self.tris[besttri]
            scores[besttri] = -666.0
            trischedule.append(tri)
            for v in tri:
                if v.cacherank < 0: # debug info
                    vertloads += 1  # debug info
                if v.index < 0:
                    v.index = len(vertschedule)
                    vertschedule.append(v)
                v.uses.remove(besttri)
                v.cacherank = -1
                v.score = -1.0
            vcache = [ v for v in tri if v.uses ] + [ v for v in vcache if v.cacherank >= 0 ]
            for i, v in enumerate(vcache):
                v.cacherank = i
                v.calcScore()

            besttri = -1
            bestscore = -42.0
            for v in vcache:
                for i in v.uses:
                    v0, v1, v2 = self.tris[i]
                    scores[i] = v0.score + v1.score + v2.score
                    if scores[i] > bestscore:
                        besttri = i
                        bestscore = scores[i]
            while len(vcache) > MAXVCACHE:
                vcache.pop().cacherank = -1
            if besttri < 0:
                for i, score in enumerate(scores):
                    if score > bestscore:
                        besttri = i
                        bestscore = score

        print('%s: %d verts optimized to %d/%d loads for %d entry LRU cache' % (self.name, len(self.verts), vertloads, len(vertschedule), MAXVCACHE))
        #print('%s: %d verts scheduled to %d' % (self.name, len(self.verts), len(vertschedule)))
        self.verts = vertschedule
        # print('%s: %d tris scheduled to %d' % (self.name, len(self.tris), len(trischedule)))
        self.tris = trischedule

    def meshData(self, iqm):
        return [ iqm.addText(self.name), iqm.addText(self.material), self.firstvert, len(self.verts), self.firsttri, len(self.tris) ]


class Bone:
    def __init__(self, name, origname, index, parent, matrix):
        self.name = name
        self.origname = origname
        self.index = index
        self.parent = parent
        self.matrix = matrix
        self.localmatrix = matrix
        if self.parent:
            self.localmatrix = parent.matrix.inverted() * self.localmatrix
        self.numchannels = 0
        self.channelmask = 0
        self.channeloffsets = [ 1.0e10, 1.0e10, 1.0e10, 1.0e10, 1.0e10, 1.0e10, 1.0e10, 1.0e10, 1.0e10, 1.0e10 ]
        self.channelscales = [ -1.0e10, -1.0e10, -1.0e10, -1.0e10, -1.0e10, -1.0e10, -1.0e10, -1.0e10, -1.0e10, -1.0e10 ]

    def jointData(self, iqm):
        if self.parent:
            parent = self.parent.index
        else:
            parent = -1
        pos = self.localmatrix.to_translation()
        orient = self.localmatrix.to_quaternion()
        orient.normalize()
        if orient.w > 0:
            orient.negate()
        scale = self.localmatrix.to_scale()
        scale.x = round(scale.x*0x10000)/0x10000
        scale.y = round(scale.y*0x10000)/0x10000
        scale.z = round(scale.z*0x10000)/0x10000
        return [ iqm.addText(self.name), parent, pos.x, pos.y, pos.z, orient.x, orient.y, orient.z, orient.w, scale.x, scale.y, scale.z ]

    def poseData(self, iqm):
        if self.parent:
            parent = self.parent.index
        else:
            parent = -1
        return [ parent, self.channelmask ] + self.channeloffsets + self.channelscales

    def calcChannelMask(self):
        for i in range(0, 10):
            self.channelscales[i] -= self.channeloffsets[i]
            if self.channelscales[i] >= 1.0e-10:
                self.numchannels += 1
                self.channelmask |= 1 << i
                self.channelscales[i] /= 0xFFFF
            else:
                self.channelscales[i] = 0.0
        return self.numchannels


class Animation:
    def __init__(self, name, frames, fps = 0.0, flags = 0):
        self.name = name
        self.frames = frames
        self.fps = fps
        self.flags = flags

    def calcFrameLimits(self, bones):
        for frame in self.frames:
            for i, bone in enumerate(bones):
                loc, quat, scale, mat = frame[i]
                bone.channeloffsets[0] = min(bone.channeloffsets[0], loc.x)
                bone.channeloffsets[1] = min(bone.channeloffsets[1], loc.y)
                bone.channeloffsets[2] = min(bone.channeloffsets[2], loc.z)
                bone.channeloffsets[3] = min(bone.channeloffsets[3], quat.x)
                bone.channeloffsets[4] = min(bone.channeloffsets[4], quat.y)
                bone.channeloffsets[5] = min(bone.channeloffsets[5], quat.z)
                bone.channeloffsets[6] = min(bone.channeloffsets[6], quat.w)
                bone.channeloffsets[7] = min(bone.channeloffsets[7], scale.x)
                bone.channeloffsets[8] = min(bone.channeloffsets[8], scale.y)
                bone.channeloffsets[9] = min(bone.channeloffsets[9], scale.z)
                bone.channelscales[0] = max(bone.channelscales[0], loc.x)
                bone.channelscales[1] = max(bone.channelscales[1], loc.y)
                bone.channelscales[2] = max(bone.channelscales[2], loc.z)
                bone.channelscales[3] = max(bone.channelscales[3], quat.x)
                bone.channelscales[4] = max(bone.channelscales[4], quat.y)
                bone.channelscales[5] = max(bone.channelscales[5], quat.z)
                bone.channelscales[6] = max(bone.channelscales[6], quat.w)
                bone.channelscales[7] = max(bone.channelscales[7], scale.x)
                bone.channelscales[8] = max(bone.channelscales[8], scale.y)
                bone.channelscales[9] = max(bone.channelscales[9], scale.z)

    def animData(self, iqm):
        return [ iqm.addText(self.name), self.firstframe, len(self.frames), self.fps, self.flags ]

    def frameData(self, bones):
        data = b''
        for frame in self.frames:
            for i, bone in enumerate(bones):
                loc, quat, scale, mat = frame[i]
                if (bone.channelmask&0x7F) == 0x7F:
                    lx = int(round((loc.x - bone.channeloffsets[0]) / bone.channelscales[0]))
                    ly = int(round((loc.y - bone.channeloffsets[1]) / bone.channelscales[1]))
                    lz = int(round((loc.z - bone.channeloffsets[2]) / bone.channelscales[2]))
                    qx = int(round((quat.x - bone.channeloffsets[3]) / bone.channelscales[3]))
                    qy = int(round((quat.y - bone.channeloffsets[4]) / bone.channelscales[4]))
                    qz = int(round((quat.z - bone.channeloffsets[5]) / bone.channelscales[5]))
                    qw = int(round((quat.w - bone.channeloffsets[6]) / bone.channelscales[6]))
                    data += struct.pack('<7H', lx, ly, lz, qx, qy, qz, qw)
                else:
                    if bone.channelmask & 1:
                        data += struct.pack('<H', int(round((loc.x - bone.channeloffsets[0]) / bone.channelscales[0])))
                    if bone.channelmask & 2:
                        data += struct.pack('<H', int(round((loc.y - bone.channeloffsets[1]) / bone.channelscales[1])))
                    if bone.channelmask & 4:
                        data += struct.pack('<H', int(round((loc.z - bone.channeloffsets[2]) / bone.channelscales[2])))
                    if bone.channelmask & 8:
                        data += struct.pack('<H', int(round((quat.x - bone.channeloffsets[3]) / bone.channelscales[3])))
                    if bone.channelmask & 16:
                        data += struct.pack('<H', int(round((quat.y - bone.channeloffsets[4]) / bone.channelscales[4])))
                    if bone.channelmask & 32:
                        data += struct.pack('<H', int(round((quat.z - bone.channeloffsets[5]) / bone.channelscales[5])))
                    if bone.channelmask & 64:
                        data += struct.pack('<H', int(round((quat.w - bone.channeloffsets[6]) / bone.channelscales[6])))
                if bone.channelmask & 128:
                    data += struct.pack('<H', int(round((scale.x - bone.channeloffsets[7]) / bone.channelscales[7])))
                if bone.channelmask & 256:
                    data += struct.pack('<H', int(round((scale.y - bone.channeloffsets[8]) / bone.channelscales[8])))
                if bone.channelmask & 512:
                    data += struct.pack('<H', int(round((scale.z - bone.channeloffsets[9]) / bone.channelscales[9])))
        return data

    def frameBoundsData(self, bones, meshes, frame, invbase):
        bbmin = bbmax = None
        xyradius = 0.0
        radius = 0.0
        transforms = []
        for i, bone in enumerate(bones):
            loc, quat, scale, mat = frame[i]
            if bone.parent:
                mat = transforms[bone.parent.index] * mat
            transforms.append(mat)
        for i, mat in enumerate(transforms):
            transforms[i] = mat * invbase[i]
        for mesh in meshes:
            for v in mesh.verts:
                pos = mathutils.Vector((0.0, 0.0, 0.0))
                for (weight, bone) in v.weights:
                    if weight > 0:
                        pos += (transforms[bone] * v.coord) * (weight / 255.0)
                if bbmin:
                    bbmin.x = min(bbmin.x, pos.x)
                    bbmin.y = min(bbmin.y, pos.y)
                    bbmin.z = min(bbmin.z, pos.z)
                    bbmax.x = max(bbmax.x, pos.x)
                    bbmax.y = max(bbmax.y, pos.y)
                    bbmax.z = max(bbmax.z, pos.z)
                else:
                    bbmin = pos.copy()
                    bbmax = pos.copy()
                pradius = pos.x*pos.x + pos.y*pos.y
                if pradius > xyradius:
                    xyradius = pradius
                pradius += pos.z*pos.z
                if pradius > radius:
                    radius = pradius
        if bbmin:
            xyradius = math.sqrt(xyradius)
            radius = math.sqrt(radius)
        else:
            bbmin = bbmax = mathutils.Vector((0.0, 0.0, 0.0))
        return IQM_BOUNDS.pack(bbmin.x, bbmin.y, bbmin.z, bbmax.x, bbmax.y, bbmax.z, xyradius, radius)

    def boundsData(self, bones, meshes):
        invbase = []
        for bone in bones:
            invbase.append(bone.matrix.inverted())
        data = b''
        for i, frame in enumerate(self.frames):
            print('Calculating bounding box for %s:%d' % (self.name, i))
            data += self.frameBoundsData(bones, meshes, frame, invbase)
        return data

class IQMFile:
    def __init__(self):
        self.textoffsets = {}
        self.textdata = b''
        self.meshes = []
        self.meshdata = []
        self.numverts = 0
        self.numtris = 0
        self.joints = []
        self.jointdata = []
        self.numframes = 0
        self.framesize = 0
        self.anims = []
        self.posedata = []
        self.animdata = []
        self.framedata = []
        self.vertdata = []

    def addText(self, str):
        if not self.textdata:
            self.textdata += b'\x00'
            self.textoffsets[''] = 0
        try:
            return self.textoffsets[str]
        except:
            offset = len(self.textdata)
            self.textoffsets[str] = offset
            self.textdata += bytes(str, encoding="utf8") + b'\x00'
            return offset

    def addJoints(self, bones):
        for bone in bones:
            self.joints.append(bone)
            if self.meshes:
                self.jointdata.append(bone.jointData(self))

    def addMeshes(self, meshes):
        self.meshes += meshes
        for mesh in meshes:
            mesh.firstvert = self.numverts
            mesh.firsttri = self.numtris
            self.meshdata.append(mesh.meshData(self))
            self.numverts += len(mesh.verts)
            self.numtris += len(mesh.tris)

    def addAnims(self, anims):
        self.anims += anims
        for anim in anims:
            anim.firstframe = self.numframes
            self.animdata.append(anim.animData(self))
            self.numframes += len(anim.frames)

    def calcFrameSize(self):
        for anim in self.anims:
            anim.calcFrameLimits(self.joints)
        self.framesize = 0
        for joint in self.joints:
            self.framesize += joint.calcChannelMask()
        for joint in self.joints:
            if self.anims:
                self.posedata.append(joint.poseData(self))
        print('Exporting %d frames of size %d' % (self.numframes, self.framesize))

    def writeVerts(self, file, offset):
        if self.numverts <= 0:
            return

        file.write(IQM_VERTEXARRAY.pack(IQM_POSITION, 0, IQM_FLOAT, 3, offset))
        offset += self.numverts * struct.calcsize('<3f')
        file.write(IQM_VERTEXARRAY.pack(IQM_TEXCOORD, 0, IQM_FLOAT, 2, offset))
        offset += self.numverts * struct.calcsize('<2f')
        file.write(IQM_VERTEXARRAY.pack(IQM_NORMAL, 0, IQM_FLOAT, 3, offset))
        offset += self.numverts * struct.calcsize('<3f')
        file.write(IQM_VERTEXARRAY.pack(IQM_TANGENT, 0, IQM_FLOAT, 4, offset))
        offset += self.numverts * struct.calcsize('<4f')
        if self.joints:
            file.write(IQM_VERTEXARRAY.pack(IQM_BLENDINDEXES, 0, IQM_UBYTE, 4, offset))
            offset += self.numverts * struct.calcsize('<4B')
            file.write(IQM_VERTEXARRAY.pack(IQM_BLENDWEIGHTS, 0, IQM_UBYTE, 4, offset))
            offset += self.numverts * struct.calcsize('<4B')
        hascolors = any(mesh.verts and mesh.verts[0].color for mesh in self.meshes)
        if hascolors:
            file.write(IQM_VERTEXARRAY.pack(IQM_COLOR, 0, IQM_UBYTE, 4, offset))
            offset += self.numverts * struct.calcsize('<4B')

        for mesh in self.meshes:
            for v in mesh.verts:
                file.write(struct.pack('<3f', *v.coord))
        for mesh in self.meshes:
            for v in mesh.verts:
                file.write(struct.pack('<2f', *v.uv))
        for mesh in self.meshes:
            for v in mesh.verts:
                file.write(struct.pack('<3f', *v.normal))
        for mesh in self.meshes:
            for v in mesh.verts:
                file.write(struct.pack('<4f', v.tangent.x, v.tangent.y, v.tangent.z, v.bitangent))
        if self.joints:
            for mesh in self.meshes:
                for v in mesh.verts:
                    file.write(struct.pack('<4B', v.weights[0][1], v.weights[1][1], v.weights[2][1], v.weights[3][1]))
            for mesh in self.meshes:
                for v in mesh.verts:
                    file.write(struct.pack('<4B', v.weights[0][0], v.weights[1][0], v.weights[2][0], v.weights[3][0]))
        if hascolors:
            for mesh in self.meshes:
                for v in mesh.verts:
                    if v.color:
                        file.write(struct.pack('<4B', v.color[0], v.color[1], v.color[2], v.color[3]))
                    else:
                        file.write(struct.pack('<4B', 0, 0, 0, 255))

    def calcNeighbors(self):
        edges = {}
        for mesh in self.meshes:
            for i, (v0, v1, v2) in enumerate(mesh.tris):
                e0 = v0.neighborKey(v1)
                e1 = v1.neighborKey(v2)
                e2 = v2.neighborKey(v0)
                tri = mesh.firsttri + i
                try: edges[e0].append(tri)
                except: edges[e0] = [tri]
                try: edges[e1].append(tri)
                except: edges[e1] = [tri]
                try: edges[e2].append(tri)
                except: edges[e2] = [tri]
        neighbors = []
        for mesh in self.meshes:
            for i, (v0, v1, v2) in enumerate(mesh.tris):
                e0 = edges[v0.neighborKey(v1)]
                e1 = edges[v1.neighborKey(v2)]
                e2 = edges[v2.neighborKey(v0)]
                tri = mesh.firsttri + i
                match0 = match1 = match2 = -1
                if len(e0) == 2: match0 = e0[e0.index(tri)^1]
                if len(e1) == 2: match1 = e1[e1.index(tri)^1]
                if len(e2) == 2: match2 = e2[e2.index(tri)^1]
                neighbors.append((match0, match1, match2))
        self.neighbors = neighbors

    def writeTris(self, file):
        for mesh in self.meshes:
            for (v0, v1, v2) in mesh.tris:
                file.write(struct.pack('<3I', v0.index + mesh.firstvert, v1.index + mesh.firstvert, v2.index + mesh.firstvert))
        for (n0, n1, n2) in self.neighbors:
            if n0 < 0: n0 = 0xFFFFFFFF
            if n1 < 0: n1 = 0xFFFFFFFF
            if n2 < 0: n2 = 0xFFFFFFFF
            file.write(struct.pack('<3I', n0, n1, n2))

    def export(self, file, usebbox = True):
        self.filesize = IQM_HEADER.size
        if self.textdata:
            while len(self.textdata) % 4:
                self.textdata += b'\x00'
            ofs_text = self.filesize
            self.filesize += len(self.textdata)
        else:
            ofs_text = 0
        if self.meshdata:
            ofs_meshes = self.filesize
            self.filesize += len(self.meshdata) * IQM_MESH.size
        else:
            ofs_meshes = 0
        if self.numverts > 0:
            ofs_vertexarrays = self.filesize
            num_vertexarrays = 4
            if self.joints:
                num_vertexarrays += 2
            hascolors = any(mesh.verts and mesh.verts[0].color for mesh in self.meshes)
            if hascolors:
                num_vertexarrays += 1
            self.filesize += num_vertexarrays * IQM_VERTEXARRAY.size
            ofs_vdata = self.filesize
            self.filesize += self.numverts * struct.calcsize('<3f2f3f4f')
            if self.joints:
                self.filesize += self.numverts * struct.calcsize('<4B4B')
            if hascolors:
                self.filesize += self.numverts * struct.calcsize('<4B')
        else:
            ofs_vertexarrays = 0
            num_vertexarrays = 0
            ofs_vdata = 0
        if self.numtris > 0:
            ofs_triangles = self.filesize
            self.filesize += self.numtris * IQM_TRIANGLE.size
            ofs_neighbors = self.filesize
            self.filesize += self.numtris * IQM_TRIANGLE.size
        else:
            ofs_triangles = 0
            ofs_neighbors = 0
        if self.jointdata:
            ofs_joints = self.filesize
            self.filesize += len(self.jointdata) * IQM_JOINT.size
        else:
            ofs_joints = 0
        if self.posedata:
            ofs_poses = self.filesize
            self.filesize += len(self.posedata) * IQM_POSE.size
        else:
            ofs_poses = 0
        if self.animdata:
            ofs_anims = self.filesize
            self.filesize += len(self.animdata) * IQM_ANIMATION.size
        else:
            ofs_anims = 0
        falign = 0
        if self.framesize * self.numframes > 0:
            ofs_frames = self.filesize
            self.filesize += self.framesize * self.numframes * struct.calcsize('<H')
            falign = (4 - (self.filesize % 4)) % 4
            self.filesize += falign
        else:
            ofs_frames = 0
        if usebbox and self.numverts > 0 and self.numframes > 0:
            ofs_bounds = self.filesize
            self.filesize += self.numframes * IQM_BOUNDS.size
        else:
            ofs_bounds = 0

        file.write(IQM_HEADER.pack('INTERQUAKEMODEL'.encode('ascii'), 2, self.filesize, 0, len(self.textdata), ofs_text, len(self.meshdata), ofs_meshes, num_vertexarrays, self.numverts, ofs_vertexarrays, self.numtris, ofs_triangles, ofs_neighbors, len(self.jointdata), ofs_joints, len(self.posedata), ofs_poses, len(self.animdata), ofs_anims, self.numframes, self.framesize, ofs_frames, ofs_bounds, 0, 0, 0, 0))
        file.write(self.textdata)
        for mesh in self.meshdata:
            file.write(IQM_MESH.pack(*mesh))
        self.writeVerts(file, ofs_vdata)
        self.writeTris(file)
        for joint in self.jointdata:
            file.write(IQM_JOINT.pack(*joint))
        for pose in self.posedata:
            file.write(IQM_POSE.pack(*pose))
        for anim in self.animdata:
            file.write(IQM_ANIMATION.pack(*anim))
        for anim in self.anims:
            file.write(anim.frameData(self.joints))
        file.write(b'\x00' * falign)
        if usebbox and self.numverts > 0 and self.numframes > 0:
            for anim in self.anims:
                file.write(anim.boundsData(self.joints, self.meshes))


def findArmature(context):
    armature = None
    for obj in context.selected_objects:
        if obj.type == 'ARMATURE':
            armature = obj
            break
    if not armature:
        for obj in context.selected_objects:
            if obj.type == 'MESH':
                armature = obj.find_armature()
                if armature:
                    break
    return armature


def derigifyBones(context, armature, scale):
    data = armature.data

    defnames = []
    orgbones = {}
    defbones = {}
    org2defs = {}
    def2org = {}
    defparent = {}
    defchildren = {}
    for bone in data.bones.values():
        if bone.name.startswith('ORG-'):
            orgbones[bone.name[4:]] = bone
            org2defs[bone.name[4:]] = []
        elif bone.name.startswith('DEF-'):
            defnames.append(bone.name[4:])
            defbones[bone.name[4:]] = bone
            defchildren[bone.name[4:]] = []
    for name, bone in defbones.items():
        orgname = name
        orgbone = orgbones.get(orgname)
        splitname = -1
        if not orgbone:
            splitname = name.rfind('.')
            suffix = ''
            if splitname >= 0 and name[splitname+1:] in [ 'l', 'r', 'L', 'R' ]:
                suffix = name[splitname:]
                splitname = name.rfind('.', 0, splitname)
            if splitname >= 0 and name[splitname+1:splitname+2].isdigit():
                orgname = name[:splitname] + suffix
                orgbone = orgbones.get(orgname)
        org2defs[orgname].append(name)
        def2org[name] = orgname
    for defs in org2defs.values():
        defs.sort()
    for name in defnames:
        bone = defbones[name]
        orgname = def2org[name]
        orgbone = orgbones.get(orgname)
        defs = org2defs[orgname]
        if orgbone:
            i = defs.index(name)
            if i == 0:
                orgparent = orgbone.parent
                if orgparent and orgparent.name.startswith('ORG-'):
                    orgpname = orgparent.name[4:]
                    defparent[name] = org2defs[orgpname][-1]
            else:
                defparent[name] = defs[i-1]
        if name in defparent:
            defchildren[defparent[name]].append(name)

    bones = {}
    worldmatrix = armature.matrix_world
    worklist = [ bone for bone in defnames if bone not in defparent ]
    for index, bname in enumerate(worklist):
        bone = defbones[bname]
        bonematrix = worldmatrix * bone.matrix_local
        if scale != 1.0:
            bonematrix.translation *= scale
        bones[bone.name] = Bone(bname, bone.name, index, bname in defparent and bones.get(defbones[defparent[bname]].name), bonematrix)
        worklist.extend(defchildren[bname])
    print('De-rigified %d bones' % len(worklist))
    return bones


def collectBones(context, armature, scale):
    data = armature.data
    bones = {}
    worldmatrix = armature.matrix_world
    worklist = [ bone for bone in data.bones.values() if not bone.parent ]
    for index, bone in enumerate(worklist):
        bonematrix = worldmatrix * bone.matrix_local
        if scale != 1.0:
            bonematrix.translation *= scale
        bones[bone.name] = Bone(bone.name, bone.name, index, bone.parent and bones.get(bone.parent.name), bonematrix)
        for child in bone.children:
            if child not in worklist:
                worklist.append(child)
    print('Collected %d bones' % len(worklist))
    return bones


def collectAnim(context, armature, scale, bones, action, startframe = None, endframe = None):
    if not startframe or not endframe:
        startframe, endframe = action.frame_range
        startframe = int(startframe)
        endframe = int(endframe)
    print('Exporting action "%s" frames %d-%d' % (action.name, startframe, endframe))
    scene = context.scene
    worldmatrix = armature.matrix_world
    armature.animation_data.action = action
    outdata = []
    for time in range(startframe, endframe+1):
        scene.frame_set(time)
        pose = armature.pose
        outframe = []
        for bone in bones:
            posematrix = pose.bones[bone.origname].matrix
            if bone.parent:
                posematrix = pose.bones[bone.parent.origname].matrix.inverted() * posematrix
            else:
                posematrix = worldmatrix * posematrix
            if scale != 1.0:
                posematrix.translation *= scale
            loc = posematrix.to_translation()
            quat = posematrix.to_quaternion()
            quat.normalize()
            if quat.w > 0:
                quat.negate()
            pscale = posematrix.to_scale()
            pscale.x = round(pscale.x*0x10000)/0x10000
            pscale.y = round(pscale.y*0x10000)/0x10000
            pscale.z = round(pscale.z*0x10000)/0x10000
            outframe.append((loc, quat, pscale, posematrix))
        outdata.append(outframe)
    return outdata


def collectAnims(context, armature, scale, bones, animspecs):
    if not armature.animation_data:
        print('Armature has no animation data')
        return []
    actions = bpy.data.actions
    animspecs = [ spec.strip() for spec in animspecs.split(',') ]
    anims = []
    scene = context.scene
    oldaction = armature.animation_data.action
    oldframe = scene.frame_current
    for animspec in animspecs:
        animspec = [ arg.strip() for arg in animspec.split(':') ]
        animname = animspec[0]
        if animname not in actions:
            print('Action "%s" not found in current armature' % animname)
            continue
        try:
            startframe = int(animspec[1])
        except:
            startframe = None
        try:
            endframe = int(animspec[2])
        except:
            endframe = None
        try:
            fps = float(animspec[3])
        except:
            fps = float(scene.render.fps)
        try:
            flags = int(animspec[4])
        except:
            flags = 0
        framedata = collectAnim(context, armature, scale, bones, actions[animname], startframe, endframe)
        anims.append(Animation(animname, framedata, fps, flags))
    armature.animation_data.action = oldaction
    scene.frame_set(oldframe)
    return anims


def collectMeshes(context, bones, scale, matfun, useskel = True, usecol = False, filetype = 'IQM'):
    vertwarn = []
    objs = context.selected_objects #context.scene.objects
    meshes = []
    for obj in objs:
        if obj.type == 'MESH':
            data = obj.to_mesh(context.scene, False, 'PREVIEW')
            if not data.polygons:
                continue
            data.calc_normals_split()
            coordmatrix = obj.matrix_world
            normalmatrix = coordmatrix.inverted().transposed()
            if scale != 1.0:
                coordmatrix = mathutils.Matrix.Scale(scale, 4) * coordmatrix
            materials = {}
            groups = obj.vertex_groups
            uvfaces = data.uv_textures.active and data.uv_textures.active.data
            uvlayer = data.uv_layers.active and data.uv_layers.active.data
            colors = None
            alpha = None
            if usecol:
                if data.vertex_colors.active:
                    if data.vertex_colors.active.name.startswith('alpha'):
                        alpha = data.vertex_colors.active.data
                    else:
                        colors = data.vertex_colors.active.data
                for layer in data.vertex_colors:
                    if layer.name.startswith('alpha'):
                        if not alpha:
                            alpha = layer.data
                    elif not colors:
                        colors = layer.data
            for face in data.polygons:
                if len(face.vertices) < 3:
                    continue

                if all([ data.vertices[i].co == data.vertices[face.vertices[0]].co for i in face.vertices[1:] ]):
                    continue

                uvface = uvfaces and uvfaces[face.index]
                material = os.path.basename(uvface.image.filepath) if uvface and uvface.image else ''
                matindex = face.material_index
                try:
                    mesh = materials[obj.name, matindex, material]
                except:
                    try:
                        matprefix = (data.materials and data.materials[matindex].name) or ''
                    except:
                        matprefix = ''
                    mesh = Mesh(obj.name, matfun(matprefix, material), data.vertices)
                    meshes.append(mesh)
                    materials[obj.name, matindex, material] = mesh

                verts = mesh.verts
                vertmap = mesh.vertmap
                faceverts = []
                for loopidx in face.loop_indices:
                    loop = data.loops[loopidx]
                    v = data.vertices[loop.vertex_index]
                    vertco = coordmatrix * v.co

                    if not face.use_smooth:
                        vertno = mathutils.Vector(face.normal)
                    else:
                        vertno = mathutils.Vector(loop.normal)
                    vertno = normalmatrix * vertno
                    vertno.normalize()

                    # flip V axis of texture space
                    if uvlayer:
                        uv = uvlayer[loopidx].uv
                        vertuv = mathutils.Vector((uv[0], 1.0 - uv[1]))
                    else:
                        vertuv = mathutils.Vector((0.0, 0.0))

                    if colors:
                        vertcol = colors[loopidx].color
                        vertcol = (int(round(vertcol[0] * 255.0)), int(round(vertcol[1] * 255.0)), int(round(vertcol[2] * 255.0)), 255)
                    else:
                        vertcol = None

                    if alpha:
                        vertalpha = alpha[loopidx].color
                        if vertcol:
                            vertcol = (vertcol[0], vertcol[1], vertcol[2], int(round(vertalpha[0] * 255.0)))
                        else:
                            vertcol = (255, 255, 255, int(round(vertalpha[0] * 255.0)))

                    vertweights = []
                    if useskel:
                        for g in v.groups:
                            try:
                                vertweights.append((g.weight, bones[groups[g.group].name].index))
                            except:
                                if (groups[g.group].name, mesh.name) not in vertwarn:
                                    vertwarn.append((groups[g.group].name, mesh.name))
                                    print('Vertex depends on non-existent bone: %s in mesh: %s' % (groups[g.group].name, mesh.name))

                    if not face.use_smooth:
                        vertindex = len(verts)
                        vertkey = Vertex(vertindex, vertco, vertno, vertuv, vertweights, vertcol)
                        if filetype == 'IQM':
                            vertkey.normalizeWeights()
                        mesh.verts.append(vertkey)
                        faceverts.append(vertkey)
                        continue

                    vertkey = Vertex(v.index, vertco, vertno, vertuv, vertweights, vertcol)
                    if filetype == 'IQM':
                        vertkey.normalizeWeights()
                    if not verts[v.index]:
                        verts[v.index] = vertkey
                        faceverts.append(vertkey)
                    elif verts[v.index] == vertkey:
                        faceverts.append(verts[v.index])
                    else:
                        try:
                            vertindex = vertmap[vertkey]
                            faceverts.append(verts[vertindex])
                        except:
                            vertindex = len(verts)
                            vertmap[vertkey] = vertindex
                            verts.append(vertkey)
                            faceverts.append(vertkey)

                # Quake winding is reversed
                for i in range(2, len(faceverts)):
                    mesh.tris.append((faceverts[0], faceverts[i], faceverts[i-1]))

    for mesh in meshes:
        mesh.optimize()
        if filetype == 'IQM':
            mesh.calcTangents()
        print('%s %s: generated %d triangles' % (mesh.name, mesh.material, len(mesh.tris)))

    return meshes


def exportIQE(file, meshes, bones, anims):
    file.write('# Inter-Quake Export\n\n')

    for bone in bones:
        if bone.parent:
            parent = bone.parent.index
        else:
            parent = -1
        file.write('joint "%s" %d\n' % (bone.name, parent))
        if meshes:
            pos = bone.localmatrix.to_translation()
            orient = bone.localmatrix.to_quaternion()
            orient.normalize()
            if orient.w > 0:
                orient.negate()
            scale = bone.localmatrix.to_scale()
            scale.x = round(scale.x*0x10000)/0x10000
            scale.y = round(scale.y*0x10000)/0x10000
            scale.z = round(scale.z*0x10000)/0x10000
            if scale.x == 1.0 and scale.y == 1.0 and scale.z == 1.0:
                file.write('\tpq %.8f %.8f %.8f %.8f %.8f %.8f %.8f\n' % (pos.x, pos.y, pos.z, orient.x, orient.y, orient.z, orient.w))
            else:
                file.write('\tpq %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f\n' % (pos.x, pos.y, pos.z, orient.x, orient.y, orient.z, orient.w, scale.x, scale.y, scale.z))

    hascolors = any(mesh.verts and mesh.verts[0].color for mesh in meshes)
    for mesh in meshes:
        file.write('\nmesh "%s"\n\tmaterial "%s"\n\n' % (mesh.name, mesh.material))
        for v in mesh.verts:
            file.write('vp %.8f %.8f %.8f\n\tvt %.8f %.8f\n\tvn %.8f %.8f %.8f\n' % (v.coord.x, v.coord.y, v.coord.z, v.uv.x, v.uv.y, v.normal.x, v.normal.y, v.normal.z))
            if bones:
                weights = '\tvb'
                for weight in v.weights:
                    weights += ' %d %.8f' % (weight[1], weight[0])
                file.write(weights + '\n')
            if hascolors:
                if v.color:
                    file.write('\tvc %.8f %.8f %.8f %.8f\n' % (v.color[0] / 255.0, v.color[1] / 255.0, v.color[2] / 255.0, v.color[3] / 255.0))
                else:
                    file.write('\tvc 0 0 0 1\n')
        file.write('\n')
        for (v0, v1, v2) in mesh.tris:
            file.write('fm %d %d %d\n' % (v0.index, v1.index, v2.index))

    for anim in anims:
        file.write('\nanimation "%s"\n\tframerate %.8f\n' % (anim.name, anim.fps))
        if anim.flags&IQM_LOOP:
            file.write('\tloop\n')
        for frame in anim.frames:
            file.write('\nframe\n')
            for (pos, orient, scale, mat) in frame:
                if scale.x == 1.0 and scale.y == 1.0 and scale.z == 1.0:
                    file.write('pq %.8f %.8f %.8f %.8f %.8f %.8f %.8f\n' % (pos.x, pos.y, pos.z, orient.x, orient.y, orient.z, orient.w))
                else:
                    file.write('pq %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f\n' % (pos.x, pos.y, pos.z, orient.x, orient.y, orient.z, orient.w, scale.x, scale.y, scale.z))

    file.write('\n')


def exportIQM(context, filename, usemesh = True, useskel = True, usebbox = True, usecol = False, scale = 1.0, animspecs = None, matfun = (lambda prefix, image: image), derigify = False, boneorder = None):
    armature = findArmature(context)
    if useskel and not armature:
        print('No armature selected')
        return

    if filename.lower().endswith('.iqm'):
        filetype = 'IQM'
    elif filename.lower().endswith('.iqe'):
        filetype = 'IQE'
    else:
        print('Unknown file type: %s' % filename)
        return

    if useskel:
        if derigify:
            bones = derigifyBones(context, armature, scale)
        else:
            bones = collectBones(context, armature, scale)
    else:
        bones = {}

    if boneorder:
        try:
            f = open(bpy_extras.io_utils.path_reference(boneorder, os.path.dirname(bpy.data.filepath), os.path.dirname(filename)), "r", encoding = "utf-8")
            names = [line.strip() for line in f.readlines()]
            f.close()
            names = [name for name in names if name in [bone.name for bone in bones.values()]]
            if len(names) != len(bones):
                print('Bone order (%d) does not match skeleton (%d)' % (len(names), len(bones)))
                return
            print('Reordering bones')
            for bone in bones.values():
                bone.index = names.index(bone.name)
        except:
            print('Failed opening bone order: %s' % boneorder)
            return

    bonelist = sorted(bones.values(), key = lambda bone: bone.index)
    if usemesh:
        meshes = collectMeshes(context, bones, scale, matfun, useskel, usecol, filetype)
    else:
        meshes = []
    if useskel and animspecs:
        anims = collectAnims(context, armature, scale, bonelist, animspecs)
    else:
        anims = []

    if filetype == 'IQM':
        iqm = IQMFile()
        iqm.addMeshes(meshes)
        iqm.addJoints(bonelist)
        iqm.addAnims(anims)
        iqm.calcFrameSize()
        iqm.calcNeighbors()

    if filename:
        try:
            if filetype == 'IQM':
                file = open(filename, 'wb')
            else:
                file = open(filename, 'w')
        except:
            print ('Failed writing to %s' % (filename))
            return
        if filetype == 'IQM':
            iqm.export(file, usebbox)
        elif filetype == 'IQE':
            exportIQE(file, meshes, bonelist, anims)
        file.close()
        print('Saved %s file to %s' % (filetype, filename))
    else:
        print('No %s file was generated' % (filetype))


class ExportIQM(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
    '''Export an Inter-Quake Model IQM or IQE file'''
    bl_idname = "export.iqm"
    bl_label = 'Export IQM'
    filename_ext = ".iqm"
    animspec = bpy.props.StringProperty(name="Animations", description="Animations to export", maxlen=1024, default="")
    usemesh = bpy.props.BoolProperty(name="Meshes", description="Generate meshes", default=True)
    useskel = bpy.props.BoolProperty(name="Skeleton", description="Generate skeleton", default=True)
    usebbox = bpy.props.BoolProperty(name="Bounding boxes", description="Generate bounding boxes", default=True)
    usecol = bpy.props.BoolProperty(name="Vertex colors", description="Export vertex colors", default=False)
    usescale = bpy.props.FloatProperty(name="Scale", description="Scale of exported model", default=1.0, min=0.0, step=50, precision=2)
    #usetrans = bpy.props.FloatVectorProperty(name="Translate", description="Translate position of exported model", step=50, precision=2, size=3)
    matfmt = bpy.props.EnumProperty(name="Materials", description="Material name format", items=[("m+i-e", "material+image-ext", ""), ("m", "material", ""), ("i", "image", "")], default="m+i-e")
    derigify = bpy.props.BoolProperty(name="De-rigify", description="Export only deformation bones from rigify", default=False)
    boneorder = bpy.props.StringProperty(name="Bone order", description="Override ordering of bones", subtype="FILE_NAME", default="")

    def execute(self, context):
        if self.properties.matfmt == "m+i-e":
            matfun = lambda prefix, image: prefix + os.path.splitext(image)[0]
        elif self.properties.matfmt == "m":
            matfun = lambda prefix, image: prefix
        else:
            matfun = lambda prefix, image: image
        exportIQM(context, self.properties.filepath, self.properties.usemesh, self.properties.useskel, self.properties.usebbox, self.properties.usecol, self.properties.usescale, self.properties.animspec, matfun, self.properties.derigify, self.properties.boneorder)
        return {'FINISHED'}

    def check(self, context):
        filepath = bpy.path.ensure_ext(self.filepath, '.iqm')
        filepathalt = bpy.path.ensure_ext(self.filepath, '.iqe')
        if filepath != self.filepath and filepathalt != self.filepath:
            self.filepath = filepath
            return True
        return False



def menu_func(self, context):
    self.layout.operator(ExportIQM.bl_idname, text="Inter-Quake Model (.iqm, .iqe)")


def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_func)

def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_func)


if __name__ == "__main__":
    register()

