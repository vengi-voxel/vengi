#ifndef UFBX_UFBX_H_INLCUDED
#define UFBX_UFBX_H_INLCUDED

// -- User configuration

#if defined(UFBX_CONFIG_HEADER)
	#include UFBX_CONFIG_HEADER
#endif

// -- Headers

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// -- Platform

#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable: 4201) // nonstandard extension used: nameless struct/union
	#pragma warning(disable: 4505) // unreferenced local function has been removed
	#define ufbx_inline static __forceinline
#elif defined(__GNUC__)
	#define ufbx_inline static inline __attribute__((always_inline, unused))
#else
	#define ufbx_inline static
#endif

#ifndef ufbx_assert
	#include <assert.h>
	#define ufbx_assert(cond) assert(cond)
#endif

// Pointer may be `NULL`.
#define ufbx_nullable

// Changing this value from default can lead into breaking API guarantees.
#define ufbx_unsafe

#ifndef ufbx_abi
	#define ufbx_abi
#endif

// -- Configuration

// TODO: Support overriding `ufbx_real` with `float` or anything else.
typedef double ufbx_real;

#define UFBX_ERROR_STACK_MAX_DEPTH 8
#define UFBX_PANIC_MESSAGE_LENGTH 128

// -- Language

#if defined(__cplusplus)
	#define UFBX_LIST_TYPE(p_name, p_type) struct p_name { p_type *data; size_t count; \
		p_type &operator[](size_t index) const { ufbx_assert(index < count); return data[index]; } \
		p_type *begin() const { return data; } \
		p_type *end() const { return data + count; } }
#else
	#define UFBX_LIST_TYPE(p_name, p_type) typedef struct p_name { p_type *data; size_t count; } p_name
#endif

#if defined(__cplusplus)
	#define UFBX_VERTEX_ATTRIB_IMPL(p_type) \
		p_type &operator[](size_t index) const { ufbx_assert(index < indices.count); return values.data[indices.data[index]]; }
#else
	#define UFBX_VERTEX_ATTRIB_IMPL(p_type)
#endif

#if defined(__cplusplus) && (__cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1900))
	#define UFBX_CALLBACK_IMPL(p_name, p_fn, p_params, p_args) \
		p_name() = default; \
		p_name(p_fn *f) : fn(f), user(nullptr) { } \
		template <typename F> p_name(F &f) \
			: fn([] p_params { F *f = (F*)user; return f p_args; }), user((void*)&f) { }
#else
	#define UFBX_CALLBACK_IMPL(p_name, p_fn, p_params, p_args)
#endif

// -- Version

#define ufbx_pack_version(major, minor, patch) ((uint32_t)(major)*1000000u + (uint32_t)(minor)*1000u + (uint32_t)(patch))
#define ufbx_version_major(version) ((uint32_t)(version)/1000000u%1000u)
#define ufbx_version_minor(version) ((uint32_t)(version)/1000u%1000u)
#define ufbx_version_patch(version) ((uint32_t)(version)%1000u)

#define UFBX_HEADER_VERSION ufbx_pack_version(0, 1, 1)

// -- Basic types

#define UFBX_NO_INDEX ((uint32_t)~0u)

// Null-terminated UTF-8 encoded string within an FBX file
typedef struct ufbx_string {
	const char *data;
	size_t length;
} ufbx_string;

// Opaque byte buffer blob
typedef struct ufbx_blob {
	const void *data;
	size_t size;
} ufbx_blob;

// 2D vector
typedef struct ufbx_vec2 {
	union {
		struct { ufbx_real x, y; };
		ufbx_real v[2];
	};
} ufbx_vec2;

// 3D vector
typedef struct ufbx_vec3 {
	union {
		struct { ufbx_real x, y, z; };
		ufbx_real v[3];
	};
} ufbx_vec3;

// 4D vector
typedef struct ufbx_vec4 {
	union {
		struct { ufbx_real x, y, z, w; };
		ufbx_real v[4];
	};
} ufbx_vec4;

// Quaternion
typedef struct ufbx_quat {
	union {
		struct { ufbx_real x, y, z, w; };
		ufbx_real v[4];
	};
} ufbx_quat;

// Order in which Euler-angle rotation axes are applied for a transform
// NOTE: The order in the name refers to the order of axes *applied*,
// not the multiplication order: eg. `UFBX_ROTATION_XYZ` is `Z*Y*X`
// [TODO: Figure out what the spheric rotation order is...]
typedef enum ufbx_rotation_order {
	UFBX_ROTATION_XYZ,
	UFBX_ROTATION_XZY,
	UFBX_ROTATION_YZX,
	UFBX_ROTATION_YXZ,
	UFBX_ROTATION_ZXY,
	UFBX_ROTATION_ZYX,
	UFBX_ROTATION_SPHERIC,

	UFBX_ROTATION_ORDER_COUNT,
	UFBX_ROTATION_ORDER_FORCE_32BIT = 0x7fffffff,
} ufbx_rotation_order;

// Explicit translation+rotation+scale transformation.
// NOTE: Rotation is a quaternion, not Euler angles!
typedef struct ufbx_transform {
	ufbx_vec3 translation;
	ufbx_quat rotation;
	ufbx_vec3 scale;
} ufbx_transform;

// 4x3 matrix encoding an affine transformation.
// `cols[0..2]` are the X/Y/Z basis vectors, `cols[3]` is the translation
typedef struct ufbx_matrix {
	union {
		struct {
			ufbx_real m00, m10, m20;
			ufbx_real m01, m11, m21;
			ufbx_real m02, m12, m22;
			ufbx_real m03, m13, m23;
		};
		ufbx_vec3 cols[4];
		ufbx_real v[12];
	};
} ufbx_matrix;

typedef struct ufbx_void_list {
	void *data;
	size_t count;
} ufbx_void_list;

UFBX_LIST_TYPE(ufbx_bool_list, bool);
UFBX_LIST_TYPE(ufbx_uint32_list, uint32_t);
UFBX_LIST_TYPE(ufbx_real_list, ufbx_real);
UFBX_LIST_TYPE(ufbx_vec2_list, ufbx_vec2);
UFBX_LIST_TYPE(ufbx_vec3_list, ufbx_vec3);
UFBX_LIST_TYPE(ufbx_vec4_list, ufbx_vec4);
UFBX_LIST_TYPE(ufbx_string_list, ufbx_string);

// -- Document object model

typedef enum ufbx_dom_value_type {
	UFBX_DOM_VALUE_NUMBER,
	UFBX_DOM_VALUE_STRING,
	UFBX_DOM_VALUE_ARRAY_I8,
	UFBX_DOM_VALUE_ARRAY_I32,
	UFBX_DOM_VALUE_ARRAY_I64,
	UFBX_DOM_VALUE_ARRAY_F32,
	UFBX_DOM_VALUE_ARRAY_F64,
	UFBX_DOM_VALUE_ARRAY_RAW_STRING,
	UFBX_DOM_VALUE_ARRAY_IGNORED,

	UFBX_DOM_VALUE_TYPE_COUNT,
	UFBX_DOM_VALUE_TYPE_FORCE_32BIT = 0x7fffffff,
} ufbx_dom_value_type;

typedef struct ufbx_dom_node ufbx_dom_node;

typedef struct ufbx_dom_value {
	ufbx_dom_value_type type;
	ufbx_string value_str;
	ufbx_blob value_blob;
	int64_t value_int;
	double value_float;
} ufbx_dom_value;

UFBX_LIST_TYPE(ufbx_dom_node_list, ufbx_dom_node*);
UFBX_LIST_TYPE(ufbx_dom_value_list, ufbx_dom_value);

struct ufbx_dom_node {
	ufbx_string name;
	ufbx_dom_node_list children;
	ufbx_dom_value_list values;
};

// -- Properties

// FBX elements have properties which are arbitrary key/value pairs that can
// have inherited default values or be animated. In most cases you don't need
// to access these unless you need a feature not implemented directly in ufbx.
// NOTE: Prefer using `ufbx_find_prop[_len](...)` to search for a property by
// name as it can find it from the defaults if necessary.

typedef struct ufbx_prop ufbx_prop;
typedef struct ufbx_props ufbx_props;

// Data type contained within the property. All the data fields are always
// populated regardless of type, so there's no need to switch by type usually
// eg. `prop->value_real` and `prop->value_int` have the same value (well, close)
// if `prop->type == UFBX_PROP_INTEGER`. String values are not converted from/to.
typedef enum ufbx_prop_type {
	UFBX_PROP_UNKNOWN,
	UFBX_PROP_BOOLEAN,
	UFBX_PROP_INTEGER,
	UFBX_PROP_NUMBER,
	UFBX_PROP_VECTOR,
	UFBX_PROP_COLOR,
	UFBX_PROP_COLOR_WITH_ALPHA,
	UFBX_PROP_STRING,
	UFBX_PROP_DATE_TIME,
	UFBX_PROP_TRANSLATION,
	UFBX_PROP_ROTATION,
	UFBX_PROP_SCALING,
	UFBX_PROP_DISTANCE,
	UFBX_PROP_COMPOUND,
	UFBX_PROP_BLOB,
	UFBX_PROP_REFERENCE,

	UFBX_NUM_PROP_TYPES,
	UFBX_PROP_TYPE_FORCE_32BIT = 0x7fffffff,
} ufbx_prop_type;

// Property flags: Advanced information about properties, not usually needed.
typedef enum ufbx_prop_flags {
	// Supports animation.
	// NOTE: ufbx ignores this and allows animations on non-animatable properties.
	UFBX_PROP_FLAG_ANIMATABLE = 0x1,

	// User defined (custom) property.
	UFBX_PROP_FLAG_USER_DEFINED = 0x2,

	// Hidden in UI.
	UFBX_PROP_FLAG_HIDDEN = 0x4,

	// Disallow modification from UI for components.
	UFBX_PROP_FLAG_LOCK_X = 0x10,
	UFBX_PROP_FLAG_LOCK_Y = 0x20,
	UFBX_PROP_FLAG_LOCK_Z = 0x40,
	UFBX_PROP_FLAG_LOCK_W = 0x80,

	// Disable animation from components.
	UFBX_PROP_FLAG_MUTE_X = 0x100,
	UFBX_PROP_FLAG_MUTE_Y = 0x200,
	UFBX_PROP_FLAG_MUTE_Z = 0x400,
	UFBX_PROP_FLAG_MUTE_W = 0x800,

	// Property created by ufbx when an element has a connected `ufbx_anim_prop`
	// but doesn't contain the `ufbx_prop` it's referring to.
	// NOTE: The property may have been found in the templated defaults.
	UFBX_PROP_FLAG_SYNTHETIC = 0x1000,

	// The property has at least one `ufbx_anim_prop` in some layer.
	UFBX_PROP_FLAG_ANIMATED = 0x2000,

	// Used by `ufbx_evaluate_prop()` to indicate the the property was not found.
	UFBX_PROP_FLAG_NOT_FOUND = 0x4000,

	// The property is connected to another one.
	// This use case is relatively rare so `ufbx_prop` does not track connections
	// directly. You can find connections from `ufbx_element.connections_dst` where
	// `ufbx_connection.dst_prop` is this property and `ufbx_connection.src_prop` is defined.
	UFBX_PROP_FLAG_CONNECTED = 0x8000,

	// The value of this property is undefined (represented as zero).
	UFBX_PROP_FLAG_NO_VALUE = 0x10000,

	// This property has been overridden by the user.
	// See `ufbx_anim.prop_overrides` for more information.
	UFBX_PROP_FLAG_OVERRIDDEN = 0x20000,

	UFBX_PROP_FLAG_FORCE_32BIT = 0x7fffffff,
} ufbx_prop_flags;

// Single property with name/type/value.
struct ufbx_prop {
	ufbx_string name;
	uint32_t _internal_key;
	ufbx_prop_type type;
	ufbx_prop_flags flags;

	ufbx_string value_str;
	ufbx_blob value_blob;
	int64_t value_int;
	union {
		ufbx_real value_real_arr[4];
		ufbx_real value_real;
		ufbx_vec2 value_vec2;
		ufbx_vec3 value_vec3;
		ufbx_vec4 value_vec4;
	};
};

UFBX_LIST_TYPE(ufbx_prop_list, ufbx_prop);

// List of alphabetically sorted properties with potential defaults.
// For animated objects in as scene from `ufbx_evaluate_scene()` this list
// only has the animated properties, the originals are stored under `defaults`.
struct ufbx_props {
	ufbx_prop_list props;
	size_t num_animated;

	ufbx_nullable ufbx_props *defaults;
};

typedef struct ufbx_scene ufbx_scene;

// -- Elements

// Element is the lowest level representation of the FBX file in ufbx.
// An element contains type, id, name, and properties (see `ufbx_props` above)
// Elements may be connected to each other aribtrarily via `ufbx_connection`

typedef struct ufbx_element ufbx_element;

// Unknown
typedef struct ufbx_unknown ufbx_unknown;

// Nodes
typedef struct ufbx_node ufbx_node;

// Node attributes (common)
typedef struct ufbx_mesh ufbx_mesh;
typedef struct ufbx_light ufbx_light;
typedef struct ufbx_camera ufbx_camera;
typedef struct ufbx_bone ufbx_bone;
typedef struct ufbx_empty ufbx_empty;

// Node attributes (curves/surfaces)
typedef struct ufbx_line_curve ufbx_line_curve;
typedef struct ufbx_nurbs_curve ufbx_nurbs_curve;
typedef struct ufbx_nurbs_surface ufbx_nurbs_surface;
typedef struct ufbx_nurbs_trim_surface ufbx_nurbs_trim_surface;
typedef struct ufbx_nurbs_trim_boundary ufbx_nurbs_trim_boundary;

// Node attributes (advanced)
typedef struct ufbx_procedural_geometry ufbx_procedural_geometry;
typedef struct ufbx_stereo_camera ufbx_stereo_camera;
typedef struct ufbx_camera_switcher ufbx_camera_switcher;
typedef struct ufbx_marker ufbx_marker;
typedef struct ufbx_lod_group ufbx_lod_group;

// Deformers
typedef struct ufbx_skin_deformer ufbx_skin_deformer;
typedef struct ufbx_skin_cluster ufbx_skin_cluster;
typedef struct ufbx_blend_deformer ufbx_blend_deformer;
typedef struct ufbx_blend_channel ufbx_blend_channel;
typedef struct ufbx_blend_shape ufbx_blend_shape;
typedef struct ufbx_cache_deformer ufbx_cache_deformer;
typedef struct ufbx_cache_file ufbx_cache_file;

// Materials
typedef struct ufbx_material ufbx_material;
typedef struct ufbx_texture ufbx_texture;
typedef struct ufbx_video ufbx_video;
typedef struct ufbx_shader ufbx_shader;
typedef struct ufbx_shader_binding ufbx_shader_binding;

// Animation
typedef struct ufbx_anim_stack ufbx_anim_stack;
typedef struct ufbx_anim_layer ufbx_anim_layer;
typedef struct ufbx_anim_value ufbx_anim_value;
typedef struct ufbx_anim_curve ufbx_anim_curve;

// Collections
typedef struct ufbx_display_layer ufbx_display_layer;
typedef struct ufbx_selection_set ufbx_selection_set;
typedef struct ufbx_selection_node ufbx_selection_node;

// Constraints
typedef struct ufbx_character ufbx_character;
typedef struct ufbx_constraint ufbx_constraint;

// Miscellaneous
typedef struct ufbx_pose ufbx_pose;
typedef struct ufbx_metadata_object ufbx_metadata_object;

UFBX_LIST_TYPE(ufbx_element_list, ufbx_element*);
UFBX_LIST_TYPE(ufbx_unknown_list, ufbx_unknown*);
UFBX_LIST_TYPE(ufbx_node_list, ufbx_node*);
UFBX_LIST_TYPE(ufbx_mesh_list, ufbx_mesh*);
UFBX_LIST_TYPE(ufbx_light_list, ufbx_light*);
UFBX_LIST_TYPE(ufbx_camera_list, ufbx_camera*);
UFBX_LIST_TYPE(ufbx_bone_list, ufbx_bone*);
UFBX_LIST_TYPE(ufbx_empty_list, ufbx_empty*);
UFBX_LIST_TYPE(ufbx_line_curve_list, ufbx_line_curve*);
UFBX_LIST_TYPE(ufbx_nurbs_curve_list, ufbx_nurbs_curve*);
UFBX_LIST_TYPE(ufbx_nurbs_surface_list, ufbx_nurbs_surface*);
UFBX_LIST_TYPE(ufbx_nurbs_trim_surface_list, ufbx_nurbs_trim_surface*);
UFBX_LIST_TYPE(ufbx_nurbs_trim_boundary_list, ufbx_nurbs_trim_boundary*);
UFBX_LIST_TYPE(ufbx_procedural_geometry_list, ufbx_procedural_geometry*);
UFBX_LIST_TYPE(ufbx_stereo_camera_list, ufbx_stereo_camera*);
UFBX_LIST_TYPE(ufbx_camera_switcher_list, ufbx_camera_switcher*);
UFBX_LIST_TYPE(ufbx_marker_list, ufbx_marker*);
UFBX_LIST_TYPE(ufbx_lod_group_list, ufbx_lod_group*);
UFBX_LIST_TYPE(ufbx_skin_deformer_list, ufbx_skin_deformer*);
UFBX_LIST_TYPE(ufbx_skin_cluster_list, ufbx_skin_cluster*);
UFBX_LIST_TYPE(ufbx_blend_deformer_list, ufbx_blend_deformer*);
UFBX_LIST_TYPE(ufbx_blend_channel_list, ufbx_blend_channel*);
UFBX_LIST_TYPE(ufbx_blend_shape_list, ufbx_blend_shape*);
UFBX_LIST_TYPE(ufbx_cache_deformer_list, ufbx_cache_deformer*);
UFBX_LIST_TYPE(ufbx_cache_file_list, ufbx_cache_file*);
UFBX_LIST_TYPE(ufbx_material_list, ufbx_material*);
UFBX_LIST_TYPE(ufbx_texture_list, ufbx_texture*);
UFBX_LIST_TYPE(ufbx_video_list, ufbx_video*);
UFBX_LIST_TYPE(ufbx_shader_list, ufbx_shader*);
UFBX_LIST_TYPE(ufbx_shader_binding_list, ufbx_shader_binding*);
UFBX_LIST_TYPE(ufbx_anim_stack_list, ufbx_anim_stack*);
UFBX_LIST_TYPE(ufbx_anim_layer_list, ufbx_anim_layer*);
UFBX_LIST_TYPE(ufbx_anim_value_list, ufbx_anim_value*);
UFBX_LIST_TYPE(ufbx_anim_curve_list, ufbx_anim_curve*);
UFBX_LIST_TYPE(ufbx_display_layer_list, ufbx_display_layer*);
UFBX_LIST_TYPE(ufbx_selection_set_list, ufbx_selection_set*);
UFBX_LIST_TYPE(ufbx_selection_node_list, ufbx_selection_node*);
UFBX_LIST_TYPE(ufbx_character_list, ufbx_character*);
UFBX_LIST_TYPE(ufbx_constraint_list, ufbx_constraint*);
UFBX_LIST_TYPE(ufbx_pose_list, ufbx_pose*);
UFBX_LIST_TYPE(ufbx_metadata_object_list, ufbx_metadata_object*);

typedef enum ufbx_element_type {
	UFBX_ELEMENT_UNKNOWN,             // < `ufbx_unknown`
	UFBX_ELEMENT_NODE,                // < `ufbx_node`
	UFBX_ELEMENT_MESH,                // < `ufbx_mesh`
	UFBX_ELEMENT_LIGHT,               // < `ufbx_light`
	UFBX_ELEMENT_CAMERA,              // < `ufbx_camera`
	UFBX_ELEMENT_BONE,                // < `ufbx_bone`
	UFBX_ELEMENT_EMPTY,               // < `ufbx_empty`
	UFBX_ELEMENT_LINE_CURVE,          // < `ufbx_line_curve`
	UFBX_ELEMENT_NURBS_CURVE,         // < `ufbx_nurbs_curve`
	UFBX_ELEMENT_NURBS_SURFACE,       // < `ufbx_nurbs_surface`
	UFBX_ELEMENT_NURBS_TRIM_SURFACE,  // < `ufbx_nurbs_trim_surface`
	UFBX_ELEMENT_NURBS_TRIM_BOUNDARY, // < `ufbx_nurbs_trim_boundary`
	UFBX_ELEMENT_PROCEDURAL_GEOMETRY, // < `ufbx_procedural_geometry`
	UFBX_ELEMENT_STEREO_CAMERA,       // < `ufbx_stereo_camera`
	UFBX_ELEMENT_CAMERA_SWITCHER,     // < `ufbx_camera_switcher`
	UFBX_ELEMENT_MARKER,              // < `ufbx_marker`
	UFBX_ELEMENT_LOD_GROUP,           // < `ufbx_lod_group`
	UFBX_ELEMENT_SKIN_DEFORMER,       // < `ufbx_skin_deformer`
	UFBX_ELEMENT_SKIN_CLUSTER,        // < `ufbx_skin_cluster`
	UFBX_ELEMENT_BLEND_DEFORMER,      // < `ufbx_blend_deformer`
	UFBX_ELEMENT_BLEND_CHANNEL,       // < `ufbx_blend_channel`
	UFBX_ELEMENT_BLEND_SHAPE,         // < `ufbx_blend_shape`
	UFBX_ELEMENT_CACHE_DEFORMER,      // < `ufbx_cache_deformer`
	UFBX_ELEMENT_CACHE_FILE,          // < `ufbx_cache_file`
	UFBX_ELEMENT_MATERIAL,            // < `ufbx_material`
	UFBX_ELEMENT_TEXTURE,             // < `ufbx_texture`
	UFBX_ELEMENT_VIDEO,               // < `ufbx_video`
	UFBX_ELEMENT_SHADER,              // < `ufbx_shader`
	UFBX_ELEMENT_SHADER_BINDING,      // < `ufbx_shader_binding`
	UFBX_ELEMENT_ANIM_STACK,          // < `ufbx_anim_stack`
	UFBX_ELEMENT_ANIM_LAYER,          // < `ufbx_anim_layer`
	UFBX_ELEMENT_ANIM_VALUE,          // < `ufbx_anim_value`
	UFBX_ELEMENT_ANIM_CURVE,          // < `ufbx_anim_curve`
	UFBX_ELEMENT_DISPLAY_LAYER,       // < `ufbx_display_layer`
	UFBX_ELEMENT_SELECTION_SET,       // < `ufbx_selection_set`
	UFBX_ELEMENT_SELECTION_NODE,      // < `ufbx_selection_node`
	UFBX_ELEMENT_CHARACTER,           // < `ufbx_character`
	UFBX_ELEMENT_CONSTRAINT,          // < `ufbx_constraint`
	UFBX_ELEMENT_POSE,                // < `ufbx_pose`
	UFBX_ELEMENT_METADATA_OBJECT,     // < `ufbx_metadata_object`

	UFBX_ELEMENT_TYPE_COUNT,
	UFBX_ELEMENT_TYPE_FORCE_32BIT = 0x7fffffff,
	UFBX_ELEMENT_TYPE_FIRST_ATTRIB = UFBX_ELEMENT_MESH,
	UFBX_ELEMENT_TYPE_LAST_ATTRIB = UFBX_ELEMENT_LOD_GROUP,
} ufbx_element_type;

// Connection between two elements.
// Source and destination are somewhat arbitrary but the destination is
// often the "container" like a parent node or mesh containing a deformer.
typedef struct ufbx_connection {
	ufbx_element *src;
	ufbx_element *dst;
	ufbx_string src_prop;
	ufbx_string dst_prop;
} ufbx_connection;

UFBX_LIST_TYPE(ufbx_connection_list, ufbx_connection);

// Element "base-class" common to each element.
// Some fields (like `connections_src`) are advanced and not visible
// in the specialized element structs.
// NOTE: The `element_id` value is consistent when loading the
// _same_ file, but re-exporting the file will invalidate them. (TOMOVE)
struct ufbx_element {
	ufbx_string name;
	ufbx_props props;
	uint32_t element_id;
	uint32_t typed_id;
	ufbx_node_list instances;
	ufbx_element_type type;
	ufbx_connection_list connections_src;
	ufbx_connection_list connections_dst;
	ufbx_nullable ufbx_dom_node *dom_node;
	ufbx_scene *scene;
};

// -- Unknown

struct ufbx_unknown {
	// Shared "base-class" header, see `ufbx_element`.
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// FBX format specific type information.
	// In ASCII FBX format:
	//   super_type: ID, "type::name", "sub_type" { ... }
	ufbx_string type;
	ufbx_string super_type;
	ufbx_string sub_type;
};

// -- Nodes

// Inherit type specifies how hierarchial node transforms are combined.
// `UFBX_INHERIT_NORMAL` is combined using the "proper" multiplication
// `UFBX_INHERIT_NO_SHEAR` does component-wise { pos+pos, rot*rot, scale*scale }
// `UFBX_INHERIT_NO_SCALE` ignores the parent scale { pos+pos, rot*rot, scale }
typedef enum ufbx_inherit_type {
	UFBX_INHERIT_NO_SHEAR, // R*r*S*s
	UFBX_INHERIT_NORMAL,   // R*S*r*s
	UFBX_INHERIT_NO_SCALE, // R*r*s

	UFBX_INHERIT_TYPE_COUNT,
	UFBX_INHERIT_TYPE_FORCE_32BIT = 0x7fffffff,
} ufbx_inherit_type;

// Nodes form the scene transformation hierarchy and can contain attached
// elements such as meshes or lights. In normal cases a single `ufbx_node`
// contains only a single attached element, so using `type/mesh/...` is safe.
struct ufbx_node {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// Node hierarchy

	// Parent node containing this one if not root.
	//
	// Always non-`NULL` for non-root nodes unless
	// `ufbx_load_opts.allow_nodes_out_of_root` is enabled.
	ufbx_nullable ufbx_node *parent;

	// List of child nodes parented to this node.
	ufbx_node_list children;

	// Attached element type and typed pointers.
	//
	// Set to `NULL` if not in use, so checking `attrib_type` is not required.
	ufbx_nullable ufbx_mesh *mesh;
	ufbx_nullable ufbx_light *light;
	ufbx_nullable ufbx_camera *camera;
	ufbx_nullable ufbx_bone *bone;

	// Less common attributes use these fields.
	//
	// Defined even if it is one of the above, eg. `ufbx_mesh`. In case there
	// is multiple attributes this will be the first one.
	ufbx_nullable ufbx_element *attrib;

	// `attrib->type` if `attrib` is defined, otherwise `UFBX_ELEMENT_UNKNOWN`.
	ufbx_element_type attrib_type;

	// List of _all_ attached attribute elements.
	//
	// In most cases there is only zero or one attributes per node, but if you
	// have a very exotic FBX file nodes may have multiple attributes.
	ufbx_element_list all_attribs;

	// Local transform in parent, geometry transform is a non-inherited
	// transform applied only to attachments like meshes
	ufbx_inherit_type inherit_type;
	ufbx_transform local_transform;
	ufbx_transform geometry_transform;

	// Raw Euler angles in degrees for those who want them

	// Specifies the axis order `euler_rotation` is applied in.
	ufbx_rotation_order rotation_order;
	// Rotation around the local X/Y/Z axes in `rotation_order`.
	// The angles are specified in degrees.
	ufbx_vec3 euler_rotation;

	// Transform to the global "world" space, may be incorrect if the node
	// uses `UFBX_INHERIT_NORMAL`, prefer using the `node_to_world` matrix.
	ufbx_transform world_transform;

	// Matrices derived from the transformations, for transforming geometry
	// prefer using `geometry_to_world` as that supports geometric transforms.

	// Transform from this node to `parent` space.
	// Equivalent to `ufbx_transform_to_matrix(&local_transform)`.
	ufbx_matrix node_to_parent;
	// Transform from this node to the world space, ie. multiplying all the
	// `node_to_parent` matrices of the parent chain together.
	// NOTE: Not the same as `ufbx_transform_to_matrix(&world_transform)`
	// as this matrix will account for potential shear (if `inherit_type == UFBX_INHERIT_NORMAL`).
	ufbx_matrix node_to_world;
	// Transform from the attribute to this node. Does not affect the transforms
	// of `children`!
	// Equivalent to `ufbx_transform_to_matrix(&geometry_transform)`.
	ufbx_matrix geometry_to_node;
	// Transform from attribute space to world space.
	// Equivalent to `ufbx_matrix_mul(&node_to_world, &geometry_to_node)`.
	ufbx_matrix geometry_to_world;

	// Materials used by `mesh` or other `attrib`.
	// There may be multiple copies of a single `ufbx_mesh` with different materials
	// in the `ufbx_node` instances.
	ufbx_material_list materials;

	// Visibility state.
	bool visible;

	// True if this node is the implicit root node of the scene.
	bool is_root;

	// How deep is this node in the parent hierarchy. Root node is at depth `0`
	// and the immediate children of root at `1`.
	uint32_t node_depth;
};

// Vertex attribute: All attributes are stored in a consistent indexed format
// regardless of how it's actually stored in the file.
//
// `values` is a contiguous array of attribute values.
// `indices` maps each mesh index into a value in the `values` array.
//
// If `unique_per_vertex` is set then the attribute is guaranteed to have a
// single defined value per vertex accessible via:
//   attrib.values.data[attrib.indices.data[mesh->vertex_first_index[vertex_ix]]
typedef struct ufbx_vertex_attrib {
	bool exists;
	ufbx_void_list values;
	ufbx_uint32_list indices;
	size_t value_reals;
	bool unique_per_vertex;
} ufbx_vertex_attrib;

// 1D vertex attribute, see `ufbx_vertex_attrib` for information
typedef struct ufbx_vertex_real {
	bool exists;
	ufbx_real_list values;
	ufbx_uint32_list indices;
	size_t value_reals;
	bool unique_per_vertex;

	UFBX_VERTEX_ATTRIB_IMPL(ufbx_real)
} ufbx_vertex_real;

// 2D vertex attribute, see `ufbx_vertex_attrib` for information
typedef struct ufbx_vertex_vec2 {
	bool exists;
	ufbx_vec2_list values;
	ufbx_uint32_list indices;
	size_t value_reals;
	bool unique_per_vertex;

	UFBX_VERTEX_ATTRIB_IMPL(ufbx_vec2)
} ufbx_vertex_vec2;

// 3D vertex attribute, see `ufbx_vertex_attrib` for information
typedef struct ufbx_vertex_vec3 {
	bool exists;
	ufbx_vec3_list values;
	ufbx_uint32_list indices;
	size_t value_reals;
	bool unique_per_vertex;

	UFBX_VERTEX_ATTRIB_IMPL(ufbx_vec3)
} ufbx_vertex_vec3;

// 4D vertex attribute, see `ufbx_vertex_attrib` for information
typedef struct ufbx_vertex_vec4 {
	bool exists;
	ufbx_vec4_list values;
	ufbx_uint32_list indices;
	size_t value_reals;
	bool unique_per_vertex;

	UFBX_VERTEX_ATTRIB_IMPL(ufbx_vec4)
} ufbx_vertex_vec4;

// Vertex UV set/layer
typedef struct ufbx_uv_set {
	ufbx_string name;
	uint32_t index;

	// Vertex attributes, see `ufbx_mesh` attributes for more information
	ufbx_vertex_vec2 vertex_uv;        // < UV / texture coordinates
	ufbx_vertex_vec3 vertex_tangent;   // < (optional) Tangent vector in UV.x direction
	ufbx_vertex_vec3 vertex_bitangent; // < (optional) Tangent vector in UV.y direction
} ufbx_uv_set;

// Vertex color set/layer
typedef struct ufbx_color_set {
	ufbx_string name;
	uint32_t index;

	// Vertex attributes, see `ufbx_mesh` attributes for more information
	ufbx_vertex_vec4 vertex_color; // < Per-vertex RGBA color
} ufbx_color_set;

UFBX_LIST_TYPE(ufbx_uv_set_list, ufbx_uv_set);
UFBX_LIST_TYPE(ufbx_color_set_list, ufbx_color_set);

// Edge between two _indices_ in a mesh
typedef struct ufbx_edge {
	union {
		struct { uint32_t a, b; };
		uint32_t indices[2];
	};
} ufbx_edge;

UFBX_LIST_TYPE(ufbx_edge_list, ufbx_edge);

// Polygonal face with arbitrary number vertices, a single face contains a 
// contiguous range of mesh indices, eg. `{5,3}` would have indices 5, 6, 7
//
// NOTE: `num_indices` maybe less than 3 in which case the face is invalid!
// [TODO #23: should probably remove the bad faces at load time]
typedef struct ufbx_face {
	uint32_t index_begin;
	uint32_t num_indices;
} ufbx_face;

UFBX_LIST_TYPE(ufbx_face_list, ufbx_face);

typedef struct ufbx_mesh_material {
	ufbx_nullable ufbx_material *material;

	// Sub-set of the geometry that uses this specific material
	size_t num_faces;     // < Number of faces (polygons) using this material
	size_t num_triangles; // < Number of triangles using this material if triangulated

	// Indices to `ufbx_mesh.faces[]` that use this material.
	// Always contains `num_faces` elements.
	ufbx_uint32_list face_indices; 

} ufbx_mesh_material;

UFBX_LIST_TYPE(ufbx_mesh_material_list, ufbx_mesh_material);

typedef struct ufbx_subdivision_weight_range {
	uint32_t weight_begin;
	uint32_t num_weights;
} ufbx_subdivision_weight_range;

UFBX_LIST_TYPE(ufbx_subdivision_weight_range_list, ufbx_subdivision_weight_range);

typedef struct ufbx_subdivision_weight {
	ufbx_real weight;
	uint32_t index;
} ufbx_subdivision_weight;

UFBX_LIST_TYPE(ufbx_subdivision_weight_list, ufbx_subdivision_weight);

typedef struct ufbx_subdivision_result {
	size_t result_memory_used;
	size_t temp_memory_used;
	size_t result_allocs;
	size_t temp_allocs;

	// Weights of vertices in the source model.
	// Defined if `ufbx_subdivide_opts.evaluate_source_vertices` is set.
	ufbx_subdivision_weight_range_list source_vertex_ranges;
	ufbx_subdivision_weight_list source_vertex_weights;

	// Weights of skin clusters in the source model.
	// Defined if `ufbx_subdivide_opts.evaluate_source_vertices` is set.
	ufbx_subdivision_weight_range_list skin_cluster_ranges;
	ufbx_subdivision_weight_list skin_cluster_weights;

} ufbx_subdivision_result;

typedef enum ufbx_subdivision_display_mode {
	UFBX_SUBDIVISION_DISPLAY_DISABLED,
	UFBX_SUBDIVISION_DISPLAY_HULL,
	UFBX_SUBDIVISION_DISPLAY_HULL_AND_SMOOTH,
	UFBX_SUBDIVISION_DISPLAY_SMOOTH,

	UFBX_SUBDIVISION_DISPLAY_MODE_COUNT,
	UFBX_SUBDIVISION_DISPLAY_MODE_FORCE_32BIT = 0x7fffffff,
} ufbx_subdivision_display_mode;

typedef enum ufbx_subdivision_boundary {
	UFBX_SUBDIVISION_BOUNDARY_DEFAULT,
	UFBX_SUBDIVISION_BOUNDARY_LEGACY,
	// OpenSubdiv: `VTX_BOUNDARY_EDGE_AND_CORNER` / `FVAR_LINEAR_CORNERS_ONLY`
	UFBX_SUBDIVISION_BOUNDARY_SHARP_CORNERS,
	// OpenSubdiv: `VTX_BOUNDARY_EDGE_ONLY` / `FVAR_LINEAR_NONE`
	UFBX_SUBDIVISION_BOUNDARY_SHARP_NONE,
	// OpenSubdiv: `FVAR_LINEAR_BOUNDARIES`
	UFBX_SUBDIVISION_BOUNDARY_SHARP_BOUNDARY,
	// OpenSubdiv: `FVAR_LINEAR_ALL`
	UFBX_SUBDIVISION_BOUNDARY_SHARP_INTERIOR,

	UFBX_SUBDIVISION_BOUNDARY_COUNT,
	UFBX_SUBDIVISION_BOUNDARY_FORCE_32BIT = 0x7fffffff,
} ufbx_subdivision_boundary;

// Polygonal mesh geometry.
//
// Example mesh with two triangles (x, z) and a quad (y).
// The faces have a constant UV coordinate x/y/z.
// The vertices have _per vertex_ normals that point up/down.
// 
//     ^   ^     ^
//     A---B-----C
//     |x /     /|
//     | /  y  / |
//     |/     / z|
//     D-----E---F
//     v     v   v
//
// Attributes may have multiple values within a single vertex, for example a
// UV seam vertex has two UV coordinates. Thus polygons are defined using
// an index that counts each corner of each face polygon. If an attribute is
// defined (even per-vertex) it will always have a valid `indices` array.
//
//   {0,3}    {3,4}    {7,3}   faces ({ index_begin, num_indices })
//   0 1 2   3 4 5 6   7 8 9   index
//   
//   0 1 3   1 2 4 3   2 4 5   vertex_indices[index]
//   A B D   B C E D   C E F   vertices[vertex_indices[index]]
//   
//   0 0 1   0 0 1 1   0 1 1   vertex_normal.indices[index]
//   ^ ^ v   ^ ^ v v   ^ v v   vertex_normal.data[vertex_normal.indices[index]]
//   
//   0 0 0   1 1 1 1   2 2 2   vertex_uv.indices[index]
//   x x x   y y y y   z z z   vertex_uv.data[vertex_uv.indices[index]]
//
// Vertex position can also be accessed uniformly through an accessor:
//   0 1 3   1 2 4 3   2 4 5   vertex_position.indices[index]
//   A B D   B C E D   C E F   vertex_position.data[vertex_position.indices[index]]
//
// Some geometry data is specified per logical vertex. Vertex positions are
// the only attribute that is guaranteed to be defined _uniquely_ per vertex.
// Vertex attributes _may_ be defined per vertex if `unique_per_vertex == true`.
// You can access the per-vertex values by first finding the first index that
// refers to the given vertex.
//
//   0 1 2 3 4 5  vertex
//   A B C D E F  vertices[vertex]
//   
//   0 1 4 2 5 9  vertex_first_index[vertex]
//   0 0 0 1 1 1  vertex_normal.indices[vertex_first_index[vertex]]
//   ^ ^ ^ v v v  vertex_normal.data[vertex_normal.indices[vertex_first_index[vertex]]]
//
struct ufbx_mesh {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };

	// Number of "logical" vertices that would be treated as a single point,
	// one vertex may be split to multiple indices for split attributes, eg. UVs
	size_t num_vertices;  // < Number of logical "vertex" points
	size_t num_indices;   // < Number of combiend vertex/attribute tuples
	size_t num_faces;     // < Number of faces (polygons) in the mesh
	size_t num_triangles; // < Number of triangles if triangulated

	// Number of edges in the mesh.
	// NOTE: May be zero in valid meshes if the file doesn't contain edge adjacency data!
	size_t num_edges;

	// Faces and optional per-face extra data
	ufbx_face_list faces;           // < Face index range
	ufbx_bool_list face_smoothing;  // < Should the face have soft normals
	ufbx_uint32_list face_material; // < Indices to `ufbx_mesh.materials` and `ufbx_node.materials`
	ufbx_uint32_list face_group;    // < Face polygon group index
	ufbx_bool_list face_hole;       // < Should the face be hidden as a "hole"
	size_t max_face_triangles;      // < Maximum number of triangles per face in this mesh
	size_t num_bad_faces;           // < Number of faces with less than 3 vertices

	// Edges and optional per-edge extra data
	ufbx_edge_list edges;           // < Edge index range
	ufbx_bool_list edge_smoothing;  // < Should the edge have soft normals
	ufbx_real_list edge_crease;     // < Crease value for subdivision surfaces
	ufbx_bool_list edge_visibility; // < Should the edge be visible

	// Logical vertices and positions, alternatively you can use
	// `vertex_position` for consistent interface with other attributes.
	ufbx_uint32_list vertex_indices;
	ufbx_vec3_list vertices;

	// First index referring to a given vertex, `UFBX_NO_INDEX` if the vertex is unused.
	ufbx_uint32_list vertex_first_index;

	// Vertex attributes, see the comment over the struct.
	//
	// NOTE: Not all meshes have all attributes, in that case `indices/data == NULL`!
	//
	// NOTE: UV/tangent/bitangent and color are the from first sets,
	// use `uv_sets/color_sets` to access the other layers.
	ufbx_vertex_vec3 vertex_position;  // < Vertex positions
	ufbx_vertex_vec3 vertex_normal;    // < (optional) Normal vectors (TODO: Generate?)
	ufbx_vertex_vec2 vertex_uv;        // < (optional) UV / texture coordinates
	ufbx_vertex_vec3 vertex_tangent;   // < (optional) Tangent vector in UV.x direction
	ufbx_vertex_vec3 vertex_bitangent; // < (optional) Tangent vector in UV.y direction
	ufbx_vertex_vec4 vertex_color;     // < (optional) Per-vertex RGBA color
	ufbx_vertex_real vertex_crease;    // < (optional) Crease value for subdivision surfaces

	// Multiple named UV/color sets
	// NOTE: The first set contains the same data as `vertex_uv/color`!
	ufbx_uv_set_list uv_sets;
	ufbx_color_set_list color_sets;

	// List of materials used by the mesh. This is a list of structures that contains
	// compact lists of face indices that use a specific material which can be more
	// useful convenient `face_material`. Use `materials[index].material` for the
	// actual material pointers.
	// NOTE: These can be wrong if you want to support per-instance materials!
	// Use `ufbx_node.materials[]` to get the per-instance materials at the same indices.
	// HINT: If this mesh has no material then `materials[]` will be empty, but if
	// you enable `ufbx_load_opts.allow_null_material` there will be a single
	// `ufbx_mesh_material` with `material == NULL` with all the faces in it.
	ufbx_mesh_material_list materials;

	// Skinned vertex positions, for efficiency the skinned positions are the
	// same as the static ones for non-skinned meshes and `skinned_is_local`
	// is set to true meaning you need to transform them manually using
	// `ufbx_transform_position(&node->geometry_to_world, skinned_pos)`!
	bool skinned_is_local;
	ufbx_vertex_vec3 skinned_position;
	ufbx_vertex_vec3 skinned_normal;

	// Deformers
	ufbx_skin_deformer_list skin_deformers;
	ufbx_blend_deformer_list blend_deformers;
	ufbx_cache_deformer_list cache_deformers;
	ufbx_element_list all_deformers;

	// Subdivision
	uint32_t subdivision_preview_levels;
	uint32_t subdivision_render_levels;
	ufbx_subdivision_display_mode subdivision_display_mode;
	ufbx_subdivision_boundary subdivision_boundary;
	ufbx_subdivision_boundary subdivision_uv_boundary;

	// Subdivision (result)
	bool subdivision_evaluated;
	ufbx_nullable ufbx_subdivision_result *subdivision_result;

	// Tessellation (result)
	bool from_tessellated_nurbs;
};

// The kind of light source
typedef enum ufbx_light_type {
	// Single point at local origin, at `node->world_transform.position`
	UFBX_LIGHT_POINT,
	// Infinite directional light pointing locally towards `light->local_direction`
	// For global: `ufbx_transform_direction(&node->node_to_world, light->local_direction)`
	UFBX_LIGHT_DIRECTIONAL,
	// Cone shaped light towards `light->local_direction`, between `light->inner/outer_angle`.
	// For global: `ufbx_transform_direction(&node->node_to_world, light->local_direction)`
	UFBX_LIGHT_SPOT,
	// Area light, shape specified by `light->area_shape`
	// TODO: Units?
	UFBX_LIGHT_AREA,
	// Volumetric light source
	// TODO: How does this work
	UFBX_LIGHT_VOLUME,

	UFBX_LIGHT_TYPE_COUNT,
	UFBX_LIGHT_TYPE_FORCE_32BIT = 0x7fffffff,
} ufbx_light_type;

// How fast does the light intensity decay at a distance
typedef enum ufbx_light_decay {
	UFBX_LIGHT_DECAY_NONE,      // < 1 (no decay)
	UFBX_LIGHT_DECAY_LINEAR,    // < 1 / d
	UFBX_LIGHT_DECAY_QUADRATIC, // < 1 / d^2 (physically accurate)
	UFBX_LIGHT_DECAY_CUBIC,     // < 1 / d^3

	UFBX_LIGHT_DECAY_COUNT,
	UFBX_LIGHT_DECAY_FORCE_32BIT = 0x7fffffff,
} ufbx_light_decay;

typedef enum ufbx_light_area_shape {
	UFBX_LIGHT_AREA_SHAPE_RECTANGLE,
	UFBX_LIGHT_AREA_SHAPE_SPHERE,

	UFBX_LIGHT_AREA_SHAPE_COUNT,
	UFBX_LIGHT_AREA_SHAPE_FORCE_32BIT = 0x7fffffff,
} ufbx_light_area_shape;

// Light source attached to a `ufbx_node`
struct ufbx_light {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };

	// Color and intensity of the light, usually you want to use `color * intensity` 
	// NOTE: `intensity` is 0.01x of the property `"Intensity"` as that matches
	// matches values in DCC programs before exporting.
	ufbx_vec3 color;
	ufbx_real intensity;

	// Direction the light is aimed at in node's local space, usually -Y
	ufbx_vec3 local_direction;

	// Type of the light and shape parameters
	ufbx_light_type type;
	ufbx_light_decay decay;
	ufbx_light_area_shape area_shape;
	ufbx_real inner_angle;
	ufbx_real outer_angle;

	bool cast_light;
	bool cast_shadows;
};

// Method of specifying the rendering resolution from properties
// NOTE: Handled internally by ufbx, ignore unless you interpret `ufbx_props` directly!
typedef enum ufbx_aspect_mode {
	// No defined resolution
	UFBX_ASPECT_MODE_WINDOW_SIZE,
	// `"AspectWidth"` and `"AspectHeight"` are relative to each other
	UFBX_ASPECT_MODE_FIXED_RATIO,
	// `"AspectWidth"` and `"AspectHeight"` are both pixels
	UFBX_ASPECT_MODE_FIXED_RESOLUTION,
	// `"AspectWidth"` is pixels, `"AspectHeight"` is relative to width
	UFBX_ASPECT_MODE_FIXED_WIDTH,
	// < `"AspectHeight"` is pixels, `"AspectWidth"` is relative to height
	UFBX_ASPECT_MODE_FIXED_HEIGHT,

	UFBX_ASPECT_MODE_COUNT,
	UFBX_ASPECT_MODE_FORCE_32BIT = 0x7fffffff,
} ufbx_aspect_mode;

// Method of specifying the field of view from properties
// NOTE: Handled internally by ufbx, ignore unless you interpret `ufbx_props` directly!
typedef enum ufbx_aperture_mode {
	// Use separate `"FieldOfViewX"` and `"FieldOfViewY"` as horizontal/vertical FOV angles
	UFBX_APERTURE_MODE_HORIZONTAL_AND_VERTICAL,
	// Use `"FieldOfView"` as horizontal FOV angle, derive vertical angle via aspect ratio
	UFBX_APERTURE_MODE_HORIZONTAL,
	// Use `"FieldOfView"` as vertical FOV angle, derive horizontal angle via aspect ratio
	UFBX_APERTURE_MODE_VERTICAL,
	// Compute the field of view from the render gate size and focal length
	UFBX_APERTURE_MODE_FOCAL_LENGTH,

	UFBX_APERTURE_MODE_COUNT,
	UFBX_APERTURE_MODE_FORCE_32BIT = 0x7fffffff,
} ufbx_aperture_mode;

// Method of specifying the render gate size from properties
// NOTE: Handled internally by ufbx, ignore unless you interpret `ufbx_props` directly!
typedef enum ufbx_gate_fit {
	// Use the film/aperture size directly as the render gate
	UFBX_GATE_FIT_NONE,
	// Fit the render gate to the height of the film, derive width from aspect ratio
	UFBX_GATE_FIT_VERTICAL,
	// Fit the render gate to the width of the film, derive height from aspect ratio
	UFBX_GATE_FIT_HORIZONTAL,
	// Fit the render gate so that it is fully contained within the film gate
	UFBX_GATE_FIT_FILL,
	// Fit the render gate so that it fully contains the film gate
	UFBX_GATE_FIT_OVERSCAN,
	// Stretch the render gate to match the film gate
	// TODO: Does this differ from `UFBX_GATE_FIT_NONE`?
	UFBX_GATE_FIT_STRETCH,

	UFBX_GATE_FIT_COUNT,
	UFBX_GATE_FIT_FORCE_32BIT = 0x7fffffff,
} ufbx_gate_fit;

// Camera film/aperture size defaults
// NOTE: Handled internally by ufbx, ignore unless you interpret `ufbx_props` directly!
typedef enum ufbx_aperture_format {
	UFBX_APERTURE_FORMAT_CUSTOM,              // < Use `"FilmWidth"` and `"FilmHeight"`
	UFBX_APERTURE_FORMAT_16MM_THEATRICAL,     // < 0.404 x 0.295 inches
	UFBX_APERTURE_FORMAT_SUPER_16MM,          // < 0.493 x 0.292 inches
	UFBX_APERTURE_FORMAT_35MM_ACADEMY,        // < 0.864 x 0.630 inches
	UFBX_APERTURE_FORMAT_35MM_TV_PROJECTION,  // < 0.816 x 0.612 inches
	UFBX_APERTURE_FORMAT_35MM_FULL_APERTURE,  // < 0.980 x 0.735 inches
	UFBX_APERTURE_FORMAT_35MM_185_PROJECTION, // < 0.825 x 0.446 inches
	UFBX_APERTURE_FORMAT_35MM_ANAMORPHIC,     // < 0.864 x 0.732 inches (squeeze ratio: 2)
	UFBX_APERTURE_FORMAT_70MM_PROJECTION,     // < 2.066 x 0.906 inches
	UFBX_APERTURE_FORMAT_VISTAVISION,         // < 1.485 x 0.991 inches
	UFBX_APERTURE_FORMAT_DYNAVISION,          // < 2.080 x 1.480 inches
	UFBX_APERTURE_FORMAT_IMAX,                // < 2.772 x 2.072 inches

	UFBX_APERTURE_FORMAT_COUNT,
	UFBX_APERTURE_FORMAT_FORCE_32BIT = 0x7fffffff,
} ufbx_aperture_format;

// Camera attached to a `ufbx_node`
struct ufbx_camera {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };

	// If set to `true`, `resolution` reprensents actual pixel values, otherwise
	// it's only useful for its aspect ratio.
	bool resolution_is_pixels;

	// Render resolution, either in pixels or arbitrary units, depending on above
	ufbx_vec2 resolution;

	// Horizontal/vertical field of view in degrees
	ufbx_vec2 field_of_view_deg;

	// Component-wise `tan(field_of_view_deg)`, also represents the size of the
	// proection frustum slice at distance of 1.
	ufbx_vec2 field_of_view_tan;

	// Advanced properties used to compute the above
	ufbx_aspect_mode aspect_mode;
	ufbx_aperture_mode aperture_mode;
	ufbx_gate_fit gate_fit;
	ufbx_aperture_format aperture_format;
	ufbx_real focal_length_mm;     // < Focal length in millimeters
	ufbx_vec2 film_size_inch;      // < Film size in inches
	ufbx_vec2 aperture_size_inch;  // < Aperture/film gate size in inches
	ufbx_real squeeze_ratio;       // < Anamoprhic stretch ratio
};

// Bone attached to a `ufbx_node`, provides the logical length of the bone
// but most interesting information is directly in `ufbx_node`.
struct ufbx_bone {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };

	// Visual radius of the bone
	ufbx_real radius;

	// Length of the bone relative to the distance between two nodes
	ufbx_real relative_length;

	// Is the bone a root bone
	bool is_root;
};

// Empty/NULL/locator connected to a node, actual details in `ufbx_node`
struct ufbx_empty {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };
};

// -- Node attributes (curves/surfaces)

// Segment of a `ufbx_line_curve`, indices refer to `ufbx_line_curve.point_indces[]`
typedef struct ufbx_line_segment {
	uint32_t index_begin;
	uint32_t num_indices;
} ufbx_line_segment;

UFBX_LIST_TYPE(ufbx_line_segment_list, ufbx_line_segment);

struct ufbx_line_curve {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };

	ufbx_vec3 color;

	ufbx_vec3_list control_points; // < List of possible values the line passes through
	ufbx_uint32_list point_indices; // < Indices to `control_points[]` the line goes through

	ufbx_line_segment_list segments;

	// Tessellation (result)
	bool from_tessellated_nurbs;
};

typedef enum ufbx_nurbs_topology {
	// The endpoints are not connected.
	UFBX_NURBS_TOPOLOGY_OPEN,
	// Repeats first `ufbx_nurbs_basis.order - 1` control points after the end.
	UFBX_NURBS_TOPOLOGY_PERIODIC,
	// Repeats the first control point after the end.
	UFBX_NURBS_TOPOLOGY_CLOSED,

	UFBX_NURBS_TOPOLOGY_COUNT,
	UFBX_NURBS_TOPOLOGY_FORCE_32BIT = 0x7fffffff,
} ufbx_nurbs_topology;

// NURBS basis functions for an axis
typedef struct ufbx_nurbs_basis {

	// Number of control points influencing a point on the curve/surface.
	// Equal to the degree plus one.
	uint32_t order;

	// Topology (periodicity) of the dimension.
	ufbx_nurbs_topology topology;

	// Subdivision of the parameter range to control points.
	ufbx_real_list knot_vector;

	// Range for the parameter value.
	ufbx_real t_min;
	ufbx_real t_max;

	// Parameter values of control points.
	ufbx_real_list spans;

	// `true` if this axis is two-dimensional.
	bool is_2d;

	// Number of control points that need to be copied to the end.
	// This is just for convenience as it could be derived from `topology` and
	// `order`. If for example `num_wrap_control_points == 3` you should repeat
	// the first 3 control points after the end.
	// HINT: You don't need to worry about this if you use ufbx functions
	// like `ufbx_evaluate_nurbs_curve_point()` as they handle this internally.
	size_t num_wrap_control_points;

	// `true` if the parametrization is well defined.
	bool valid;

} ufbx_nurbs_basis;

struct ufbx_nurbs_curve {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };

	// Basis in the U axis
	ufbx_nurbs_basis basis;

	// Linear array of control points
	// NOTE: The control points are _not_ homogenous, meaning you have to multiply
	// them by `w` before evaluating the surface.
	ufbx_vec4_list control_points;
};

struct ufbx_nurbs_surface {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };

	// Basis in the U/V axes
	ufbx_nurbs_basis basis_u;
	ufbx_nurbs_basis basis_v;

	// Number of control points for the U/V axes
	size_t num_control_points_u;
	size_t num_control_points_v;

	// 2D array of control points.
	// Memory layout: `V * num_control_points_u + U`
	// NOTE: The control points are _not_ homogenous, meaning you have to multiply
	// them by `w` before evaluating the surface.
	ufbx_vec4_list control_points;

	// How many segments tessellate each step in `ufbx_nurbs_basis.steps`.
	uint32_t span_subdivision_u;
	uint32_t span_subdivision_v;

	// If `true` the resulting normals should be flipped when evaluated.
	bool flip_normals;

	// Material for the whole surface.
	// NOTE: May be `NULL`!	
	ufbx_nullable ufbx_material *material;
};

struct ufbx_nurbs_trim_surface {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };
};

struct ufbx_nurbs_trim_boundary {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };
};

// -- Node attributes (advanced)

struct ufbx_procedural_geometry {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };
};

struct ufbx_stereo_camera {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };

	ufbx_camera *left;
	ufbx_camera *right;
};

struct ufbx_camera_switcher {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };
};

typedef enum ufbx_marker_type {
	UFBX_MARKER_UNKNOWN,     // < Unknown marker type
	UFBX_MARKER_FK_EFFECTOR, // < FK (Forward Kinematics) effector
	UFBX_MARKER_IK_EFFECTOR, // < IK (Inverse Kinematics) effector

	UFBX_MARKER_TYPE_COUNT,
	UFBX_MARKER_TYPE_FORCE_32BIT = 0x7fffffff,
} ufbx_marker_type;

// Tracking marker for effectors
struct ufbx_marker {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };

	// Type of the marker
	ufbx_marker_type type;
};

// LOD level display mode.
typedef enum ufbx_lod_display {
	UFBX_LOD_DISPLAY_USE_LOD, // < Display the LOD level if the distance is appropriate.
	UFBX_LOD_DISPLAY_SHOW,    // < Always display the LOD level.
	UFBX_LOD_DISPLAY_HIDE,    // < Never display the LOD level.

	UFBX_LOD_DISPLAY_COUNT,
	UFBX_LOD_DISPLAY_FORCE_32BIT = 0x7fffffff,
} ufbx_lod_display;

// Single LOD level within an LOD group.
// Specifies properties of the Nth child of the _node_ containing the LOD group.
typedef struct ufbx_lod_level {

	// Minimum distance to show this LOD level.
	// NOTE: In world units by default, or in screen percentage if
	// `ufbx_lod_group.relative_thresholds` is set.
	ufbx_real distance;

	// LOD display mode.
	// NOTE: Mostly for editing, you should probably ignore this
	// unless making a modeling program.
	ufbx_lod_display display;

} ufbx_lod_level;

UFBX_LIST_TYPE(ufbx_lod_level_list, ufbx_lod_level);

// Group of LOD (Level of Detail) levels for an object.
// The actual LOD models are defined in the parent `ufbx_node.children`.
struct ufbx_lod_group {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
		ufbx_node_list instances;
	}; };

	// If set to `true`, `ufbx_lod_level.distance` represents a screen size percentage.
	bool relative_distances;

	// LOD levels matching in order to `ufbx_node.children`.
	ufbx_lod_level_list lod_levels;

	// If set to `true` don't account for parent transform when computing the distance.
	bool ignore_parent_transform;

	// If `use_distance_limit` is enabled hide the group if the distance is not between
	// `distance_limit_min` and `distance_limit_max`.
	bool use_distance_limit;
	ufbx_real distance_limit_min;
	ufbx_real distance_limit_max;
};

// -- Deformers

// Method to evaluate the skinning on a per-vertex level
typedef enum ufbx_skinning_method {
	// Linear blend skinning: Blend transformation matrices by vertex weights
	UFBX_SKINNING_METHOD_LINEAR,
	// One vertex should have only one bone attached
	UFBX_SKINNING_METHOD_RIGID,
	// Convert the transformations to dual quaternions and blend in that space
	UFBX_SKINNING_METHOD_DUAL_QUATERNION,
	// Blend between `UFBX_SKINNING_METHOD_LINEAR` and `UFBX_SKINNING_METHOD_BLENDED_DQ_LINEAR`
	// The blend weight can be found either per-vertex in `ufbx_skin_vertex.dq_weight`
	// or in `ufbx_skin_deformer.dq_vertices/dq_weights` (indexed by vertex).
	UFBX_SKINNING_METHOD_BLENDED_DQ_LINEAR,

	UFBX_SKINNING_METHOD_COUNT,
	UFBX_SKINNING_METHOD_FORCE_32BIT = 0x7fffffff,
} ufbx_skinning_method;

// Skin weight information for a single mesh vertex
typedef struct ufbx_skin_vertex {

	// Each vertex is influenced by weights from `ufbx_skin_deformer.weights[]`
	// The weights are sorted by decreasing weight so you can take the first N
	// weights to get a cheaper approximation of the vertex.
	// NOTE: The weights are not guaranteed to be normalized!
	uint32_t weight_begin; // < Index to start from in the `weights[]` array
	uint32_t num_weights; // < Number of weights influencing the vertex

	// Blend weight between Linear Blend Skinning (0.0) and Dual Quaternion (1.0).
	// Should be used if `skinning_method == UFBX_SKINNING_METHOD_BLENDED_DQ_LINEAR`
	ufbx_real dq_weight;

} ufbx_skin_vertex;

UFBX_LIST_TYPE(ufbx_skin_vertex_list, ufbx_skin_vertex);

// Single per-vertex per-cluster weight, see `ufbx_skin_vertex`
typedef struct ufbx_skin_weight {
	uint32_t cluster_index; // < Index into `ufbx_skin_deformer.clusters[]`
	ufbx_real weight;       // < Amount this bone influence the vertex
} ufbx_skin_weight;

UFBX_LIST_TYPE(ufbx_skin_weight_list, ufbx_skin_weight); 

// Skin deformer specifies a binding between a logical set of bones (a skeleton)
// and a mesh. Each bone is represented by a `ufbx_skin_cluster` that contains
// the binding matrix and a `ufbx_node *bone` that has the current transformation.
struct ufbx_skin_deformer {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	ufbx_skinning_method skinning_method;

	// Clusters (bones) in the skin
	ufbx_skin_cluster_list clusters;

	// Per-vertex weight information
	ufbx_skin_vertex_list vertices;
	ufbx_skin_weight_list weights;

	// Largest amount of weights a single vertex can have
	size_t max_weights_per_vertex;

	// Blend weights between Linear Blend Skinning (0.0) and Dual Quaternion (1.0).
	// HINT: You probably want to use `vertices` and `ufbx_skin_vertex.dq_weight` instead!
	// NOTE: These may be out-of-bounds for a given mesh, `vertices` is always safe.
	size_t num_dq_weights;
	ufbx_uint32_list dq_vertices;
	ufbx_real_list dq_weights;
};

// Cluster of vertices bound to a single bone.
struct ufbx_skin_cluster {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// The bone node the cluster is attached to
	// NOTE: Always valid if found from `ufbx_skin_deformer.clusters[]` unless
	// `ufbx_load_opts.connect_broken_elements` is `true`.
	ufbx_nullable ufbx_node *bone_node;

	// Binding matrix from local mesh vertices to the bone
	ufbx_matrix geometry_to_bone; 

	// Binding matrix from local mesh _node_ to the bone.
	// NOTE: Prefer `geometry_to_bone` in most use cases!
	ufbx_matrix mesh_node_to_bone;

	// Matrix that specifies the rest/bind pose transform of the node,
	// not generally needed for skinning, use `geometry_to_bone` instead.
	ufbx_matrix bind_to_world;

	// Precomputed matrix/transform that accounts for the current bone transform
	// ie. `ufbx_matrix_mul(&cluster->bone->node_to_world, &cluster->geometry_to_bone)`
	ufbx_matrix geometry_to_world;
	ufbx_transform geometry_to_world_transform;

	// Raw weights indexed by each _vertex_ of a mesh (not index!)
	// HINT: It may be simpler to use `ufbx_skin_deformer.vertices[]/weights[]` instead!
	// NOTE: These may be out-of-bounds for a given mesh, `ufbx_skin_deformer.vertices` is always safe.
	size_t num_weights;       // < Number of vertices in the cluster
	ufbx_uint32_list vertices; // < Vertex indices in `ufbx_mesh.vertices[]`
	ufbx_real_list weights;   // < Per-vertex weight values
};

// Blend shape deformer can contain multiple channels (think of sliders between morphs)
// that may optionally have in-between keyframes.
struct ufbx_blend_deformer {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// Independent morph targets of the deformer.
	ufbx_blend_channel_list channels;
};

// Blend shape associated with a target weight in a series of morphs
typedef struct ufbx_blend_keyframe {
	// The target blend shape offsets.
	ufbx_blend_shape *shape;

	// Weight value at which to apply the keyframe at full strength
	ufbx_real target_weight;

	// The weight the shape should be currently applied with
	ufbx_real effective_weight;
} ufbx_blend_keyframe;

UFBX_LIST_TYPE(ufbx_blend_keyframe_list, ufbx_blend_keyframe);

// Blend channel consists of multiple morph-key targets that are interpolated.
// In simple cases there will be only one keyframe that is the target shape.
struct ufbx_blend_channel {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// Current weight of the channel
	ufbx_real weight;

	// Key morph targets to blend between depending on `weight`
	// In usual cases there's only one target per channel
	ufbx_blend_keyframe_list keyframes;
};

// Blend shape target containing the actual vertex offsets
struct ufbx_blend_shape {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// Vertex offsets to apply over the base mesh
	// NOTE: The `offset_vertices` may be out-of-bounds for a given mesh!
	size_t num_offsets;               // < Number of vertex offsets in the following arrays
	ufbx_uint32_list offset_vertices; // < Indices to `ufbx_mesh.vertices[]`
	ufbx_vec3_list position_offsets;  // < Always specified per-vertex offsets
	ufbx_vec3_list normal_offsets;    // < Empty if not specified
};

typedef enum ufbx_cache_file_format {
	UFBX_CACHE_FILE_FORMAT_UNKNOWN, // < Unknown cache file format
	UFBX_CACHE_FILE_FORMAT_PC2,     // < .pc2 Point cache file
	UFBX_CACHE_FILE_FORMAT_MC,      // < .mc/.mcx Maya cache file

	UFBX_CACHE_FILE_FORMAT_COUNT,
	UFBX_CACHE_FILE_FORMAT_FORCE_32BIT = 0x7fffffff,
} ufbx_cache_file_format;

typedef enum ufbx_cache_data_format {
	UFBX_CACHE_DATA_FORMAT_UNKNOWN,     // < Unknown data format
	UFBX_CACHE_DATA_FORMAT_REAL_FLOAT,  // < `float data[]`
	UFBX_CACHE_DATA_FORMAT_VEC3_FLOAT,  // < `struct { float x, y, z; } data[]`
	UFBX_CACHE_DATA_FORMAT_REAL_DOUBLE, // < `double data[]`
	UFBX_CACHE_DATA_FORMAT_VEC3_DOUBLE, // < `struct { double x, y, z; } data[]`

	UFBX_CACHE_DATA_FORMAT_COUNT,
	UFBX_CACHE_DATA_FORMAT_FORCE_32BIT = 0x7fffffff,
} ufbx_cache_data_format;

typedef enum ufbx_cache_data_encoding {
	UFBX_CACHE_DATA_ENCODING_UNKNOWN,       // < Unknown data encoding
	UFBX_CACHE_DATA_ENCODING_LITTLE_ENDIAN, // < Contiguous little-endian array
	UFBX_CACHE_DATA_ENCODING_BIG_ENDIAN,    // < Contiguous big-endian array

	UFBX_CACHE_DATA_ENCODING_COUNT,
	UFBX_CACHE_DATA_ENCODING_FORCE_32BIT = 0x7fffffff,
} ufbx_cache_data_encoding;

// Known interpretations of geometry cache data.
typedef enum ufbx_cache_interpretation {
	// Unknown interpretation, see `ufbx_cache_channel.interpretation_name` for more information.
	UFBX_CACHE_INTERPRETATION_UNKNOWN,

	// Generic "points" interpretation, FBX SDK default. Usually fine to interpret
	// as vertex positions if no other cache channels are specified.
	UFBX_CACHE_INTERPRETATION_POINTS,          

	// Vertex positions.
	UFBX_CACHE_INTERPRETATION_VERTEX_POSITION,

	// Vertex normals.
	UFBX_CACHE_INTERPRETATION_VERTEX_NORMAL,

	UFBX_CACHE_INTERPRETATION_COUNT,
	UFBX_CACHE_INTERPRETATION_FORCE_32BIT = 0x7fffffff,
} ufbx_cache_interpretation;

typedef struct ufbx_cache_frame {

	// Name of the channel this frame belongs to.
	ufbx_string channel;

	// Time of this frame in seconds.
	double time;

	// Name of the file containing the data.
	// The specified file may contain multiple frames, use `data_offset` etc. to
	// read at the right position.
	ufbx_string filename;

	// Format of the wrapper file.
	ufbx_cache_file_format file_format;

	ufbx_cache_data_format data_format;     // < Format of the data in the file
	ufbx_cache_data_encoding data_encoding; // < Binary encoding of the data
	uint64_t data_offset;                   // < Byte offset into the file
	uint32_t data_count;                    // < Number of data elements
	uint32_t data_element_bytes;            // < Size of a single data element in bytes
	uint64_t data_total_bytes;              // < Size of the whole data blob in bytes
} ufbx_cache_frame;

UFBX_LIST_TYPE(ufbx_cache_frame_list, ufbx_cache_frame);

typedef struct ufbx_cache_channel {

	// Name of the geometry cache channel.
	ufbx_string name;

	// What does the data in this channel represent.
	ufbx_cache_interpretation interpretation;

	// Source name for `interpretation`, especially useful if `interpretation` is
	// `UFBX_CACHE_INTERPRETATION_UNKNOWN`.
	ufbx_string interpretation_name;

	// List of frames belonging to this channel.
	// Sorted by time (`ufbx_cache_frame.time`).
	ufbx_cache_frame_list frames;

} ufbx_cache_channel;

UFBX_LIST_TYPE(ufbx_cache_channel_list, ufbx_cache_channel);

typedef struct ufbx_geometry_cache {
	ufbx_string root_filename;
	ufbx_cache_channel_list channels;
	ufbx_cache_frame_list frames;
	ufbx_string_list extra_info;
} ufbx_geometry_cache;

struct ufbx_cache_deformer {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	ufbx_string channel;
	ufbx_nullable ufbx_cache_file *file;

	// Only valid if `ufbx_load_opts.load_external_files` is set!
	ufbx_nullable ufbx_geometry_cache *external_cache;
	ufbx_nullable ufbx_cache_channel *external_channel;
};

struct ufbx_cache_file {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	ufbx_string filename;
	ufbx_string absolute_filename;
	ufbx_string relative_filename;

	ufbx_blob raw_filename;
	ufbx_blob raw_absolute_filename;
	ufbx_blob raw_relative_filename;

	ufbx_cache_file_format format;

	// Only valid if `ufbx_load_opts.load_external_files` is set!
	ufbx_nullable ufbx_geometry_cache *external_cache;
};

// -- Materials

// Material property, either specified with a constant value or a mapped texture
typedef struct ufbx_material_map {

	// Constant value or factor for the map.
	// May be specified simultaneously with a texture, in this case most shading models
	// use multiplicative tinting of the texture values.
	union {
		ufbx_real value_real;
		ufbx_vec2 value_vec2;
		ufbx_vec3 value_vec3;
		ufbx_vec4 value_vec4;
	};
	int64_t value_int;

	// Texture if connected, otherwise `NULL`.
	// May be valid but "disabled" (application specific) if `texture_enabled == false`.
	ufbx_nullable ufbx_texture *texture;

	// `true` if the file has specified any of the values above.
	// NOTE: The value may be set to a non-zero default even if `has_value == false`,
	// for example missing factors are set to `1.0` if a color is defined.
	bool has_value;

	// Controls whether shading should use `texture`.
	// NOTE: Some shading models allow this to be `true` even if `texture == NULL`.
	bool texture_enabled;

	// Set if the texture values should be interpreted as `1 - x`.
	// Used on various `roughness` maps when the native software uses smoothness values.
	// NOTE: Does not apply to `value_real`, ufbx inverts those values internally.
	bool texture_inverted;

	// Set to `true` if this feature should be disabled (specific to shader type).
	bool feature_disabled;

} ufbx_material_map;

// Material feature
typedef struct ufbx_material_feature_info {

	// Whether the material model uses this feature or not.
	// NOTE: The feature can be enabled but still not used if eg. the corresponding factor is at zero!
	bool enabled;

	// Explicitly enabled/disabled by the material.
	bool is_explicit;

} ufbx_material_feature_info;

// Texture attached to an FBX property
typedef struct ufbx_material_texture {
	ufbx_string material_prop; // < Name of the property in `ufbx_material.props`
	ufbx_string shader_prop;   // < Shader-specific property mapping name

	// Texture attached to the property.
	ufbx_texture *texture;

} ufbx_material_texture;

UFBX_LIST_TYPE(ufbx_material_texture_list, ufbx_material_texture);

// Shading model type
typedef enum ufbx_shader_type {
	// Unknown shading model
	UFBX_SHADER_UNKNOWN,
	// FBX builtin diffuse material
	UFBX_SHADER_FBX_LAMBERT,
	// FBX builtin diffuse+specular material
	UFBX_SHADER_FBX_PHONG,
	// Open Shading Language standard surface
	// https://github.com/Autodesk/standard-surface
	UFBX_SHADER_OSL_STANDARD_SURFACE,
	// Arnold standard surface
	// https://docs.arnoldrenderer.com/display/A5AFMUG/Standard+Surface
	UFBX_SHADER_ARNOLD_STANDARD_SURFACE,
	// 3ds Max Physical Material
	// https://knowledge.autodesk.com/support/3ds-max/learn-explore/caas/CloudHelp/cloudhelp/2022/ENU/3DSMax-Lighting-Shading/files/GUID-C1328905-7783-4917-AB86-FC3CC19E8972-htm.html
	UFBX_SHADER_3DS_MAX_PHYSICAL_MATERIAL,
	// 3ds Max PBR (Metal/Rough) material
	// https://knowledge.autodesk.com/support/3ds-max/learn-explore/caas/CloudHelp/cloudhelp/2021/ENU/3DSMax-Lighting-Shading/files/GUID-A16234A5-6500-4662-8B20-A5EC9FE1B255-htm.html
	UFBX_SHADER_3DS_MAX_PBR_METAL_ROUGH,
	// 3ds Max PBR (Spec/Gloss) material
	// https://knowledge.autodesk.com/support/3ds-max/learn-explore/caas/CloudHelp/cloudhelp/2021/ENU/3DSMax-Lighting-Shading/files/GUID-18087194-B2A6-43EF-9B80-8FD1736FAE52-htm.html
	UFBX_SHADER_3DS_MAX_PBR_SPEC_GLOSS,
	// 3ds glTF Material
	// https://help.autodesk.com/view/3DSMAX/2023/ENU/?guid=GUID-7ABFB805-1D9F-417E-9C22-704BFDF160FA
	UFBX_SHADER_GLTF_MATERIAL,
	// Stingray ShaderFX shader graph.
	// Contains a serialized `"ShaderGraph"` in `ufbx_props`.
	UFBX_SHADER_SHADERFX_GRAPH,
	// Variation of the FBX phong shader that can recover PBR properties like
	// `metalness` or `roughness` from the FBX non-physical values.
	UFBX_SHADER_BLENDER_PHONG,

	UFBX_SHADER_TYPE_COUNT,
	UFBX_SHADER_TYPE_FORCE_32BIT = 0x7fffffff,
} ufbx_shader_type;

// FBX builtin material properties, matches maps in `ufbx_material_fbx_maps`
typedef enum ufbx_material_fbx_map {
	UFBX_MATERIAL_FBX_DIFFUSE_FACTOR,
	UFBX_MATERIAL_FBX_DIFFUSE_COLOR,
	UFBX_MATERIAL_FBX_SPECULAR_FACTOR,
	UFBX_MATERIAL_FBX_SPECULAR_COLOR,
	UFBX_MATERIAL_FBX_SPECULAR_EXPONENT,
	UFBX_MATERIAL_FBX_REFLECTION_FACTOR,
	UFBX_MATERIAL_FBX_REFLECTION_COLOR,
	UFBX_MATERIAL_FBX_TRANSPARENCY_FACTOR,
	UFBX_MATERIAL_FBX_TRANSPARENCY_COLOR,
	UFBX_MATERIAL_FBX_EMISSION_FACTOR,
	UFBX_MATERIAL_FBX_EMISSION_COLOR,
	UFBX_MATERIAL_FBX_AMBIENT_FACTOR,
	UFBX_MATERIAL_FBX_AMBIENT_COLOR,
	UFBX_MATERIAL_FBX_NORMAL_MAP,
	UFBX_MATERIAL_FBX_BUMP,
	UFBX_MATERIAL_FBX_BUMP_FACTOR,
	UFBX_MATERIAL_FBX_DISPLACEMENT_FACTOR,
	UFBX_MATERIAL_FBX_DISPLACEMENT,
	UFBX_MATERIAL_FBX_VECTOR_DISPLACEMENT_FACTOR,
	UFBX_MATERIAL_FBX_VECTOR_DISPLACEMENT,

	UFBX_MATERIAL_FBX_MAP_COUNT,
	UFBX_MATERIAL_FBX_MAP_FORCE_32BIT = 0x7fffffff,
} ufbx_material_fbx_map;

// Known PBR material properties, matches maps in `ufbx_material_pbr_maps`
typedef enum ufbx_material_pbr_map {
	UFBX_MATERIAL_PBR_BASE_FACTOR,
	UFBX_MATERIAL_PBR_BASE_COLOR,
	UFBX_MATERIAL_PBR_ROUGHNESS,
	UFBX_MATERIAL_PBR_METALNESS,
	UFBX_MATERIAL_PBR_DIFFUSE_ROUGHNESS,
	UFBX_MATERIAL_PBR_SPECULAR_FACTOR,
	UFBX_MATERIAL_PBR_SPECULAR_COLOR,
	UFBX_MATERIAL_PBR_SPECULAR_IOR,
	UFBX_MATERIAL_PBR_SPECULAR_ANISOTROPY,
	UFBX_MATERIAL_PBR_SPECULAR_ROTATION,
	UFBX_MATERIAL_PBR_TRANSMISSION_FACTOR,
	UFBX_MATERIAL_PBR_TRANSMISSION_COLOR,
	UFBX_MATERIAL_PBR_TRANSMISSION_DEPTH,
	UFBX_MATERIAL_PBR_TRANSMISSION_SCATTER,
	UFBX_MATERIAL_PBR_TRANSMISSION_SCATTER_ANISOTROPY,
	UFBX_MATERIAL_PBR_TRANSMISSION_DISPERSION,
	UFBX_MATERIAL_PBR_TRANSMISSION_ROUGHNESS,
	UFBX_MATERIAL_PBR_TRANSMISSION_EXTRA_ROUGHNESS,
	UFBX_MATERIAL_PBR_TRANSMISSION_PRIORITY,
	UFBX_MATERIAL_PBR_TRANSMISSION_ENABLE_IN_AOV,
	UFBX_MATERIAL_PBR_SUBSURFACE_FACTOR,
	UFBX_MATERIAL_PBR_SUBSURFACE_COLOR,
	UFBX_MATERIAL_PBR_SUBSURFACE_RADIUS,
	UFBX_MATERIAL_PBR_SUBSURFACE_SCALE,
	UFBX_MATERIAL_PBR_SUBSURFACE_ANISOTROPY,
	UFBX_MATERIAL_PBR_SUBSURFACE_TINT_COLOR,
	UFBX_MATERIAL_PBR_SUBSURFACE_TYPE,
	UFBX_MATERIAL_PBR_SHEEN_FACTOR,
	UFBX_MATERIAL_PBR_SHEEN_COLOR,
	UFBX_MATERIAL_PBR_SHEEN_ROUGHNESS,
	UFBX_MATERIAL_PBR_COAT_FACTOR,
	UFBX_MATERIAL_PBR_COAT_COLOR,
	UFBX_MATERIAL_PBR_COAT_ROUGHNESS,
	UFBX_MATERIAL_PBR_COAT_IOR,
	UFBX_MATERIAL_PBR_COAT_ANISOTROPY,
	UFBX_MATERIAL_PBR_COAT_ROTATION,
	UFBX_MATERIAL_PBR_COAT_NORMAL,
	UFBX_MATERIAL_PBR_COAT_AFFECT_BASE_COLOR,
	UFBX_MATERIAL_PBR_COAT_AFFECT_BASE_ROUGHNESS,
	UFBX_MATERIAL_PBR_THIN_FILM_THICKNESS,
	UFBX_MATERIAL_PBR_THIN_FILM_IOR,
	UFBX_MATERIAL_PBR_EMISSION_FACTOR,
	UFBX_MATERIAL_PBR_EMISSION_COLOR,
	UFBX_MATERIAL_PBR_OPACITY,
	UFBX_MATERIAL_PBR_INDIRECT_DIFFUSE,
	UFBX_MATERIAL_PBR_INDIRECT_SPECULAR,
	UFBX_MATERIAL_PBR_NORMAL_MAP,
	UFBX_MATERIAL_PBR_TANGENT_MAP,
	UFBX_MATERIAL_PBR_DISPLACEMENT_MAP,
	UFBX_MATERIAL_PBR_MATTE_FACTOR,
	UFBX_MATERIAL_PBR_MATTE_COLOR,
	UFBX_MATERIAL_PBR_AMBIENT_OCCLUSION,

	UFBX_MATERIAL_PBR_MAP_COUNT,
	UFBX_MATERIAL_PBR_MAP_FORCE_32BIT = 0x7fffffff,
} ufbx_material_pbr_map;

// Known material features
typedef enum ufbx_material_feature {
	UFBX_MATERIAL_FEATURE_METALNESS,
	UFBX_MATERIAL_FEATURE_DIFFUSE,
	UFBX_MATERIAL_FEATURE_SPECULAR,
	UFBX_MATERIAL_FEATURE_EMISSION,
	UFBX_MATERIAL_FEATURE_TRANSMISSION,
	UFBX_MATERIAL_FEATURE_COAT,
	UFBX_MATERIAL_FEATURE_SHEEN,
	UFBX_MATERIAL_FEATURE_OPACITY,
	UFBX_MATERIAL_FEATURE_AMBIENT_OCCLUSION,
	UFBX_MATERIAL_FEATURE_MATTE,
	UFBX_MATERIAL_FEATURE_UNLIT,
	UFBX_MATERIAL_FEATURE_IOR,
	UFBX_MATERIAL_FEATURE_DIFFUSE_ROUGHNESS,
	UFBX_MATERIAL_FEATURE_TRANSMISSION_ROUGHNESS,
	UFBX_MATERIAL_FEATURE_THIN_WALLED,
	UFBX_MATERIAL_FEATURE_CAUSTICS,
	UFBX_MATERIAL_FEATURE_EXIT_TO_BACKGROUND,
	UFBX_MATERIAL_FEATURE_INTERNAL_REFLECTIONS,
	UFBX_MATERIAL_FEATURE_DOUBLE_SIDED,

	UFBX_MATERIAL_FEATURE_COUNT,
	UFBX_MATERIAL_FEATURE_FORCE_32BIT = 0x7fffffff,
} ufbx_material_feature;

typedef struct ufbx_material_fbx_maps {
	union {
		ufbx_material_map maps[UFBX_MATERIAL_FBX_MAP_COUNT];
		struct {
			ufbx_material_map diffuse_factor;
			ufbx_material_map diffuse_color;
			ufbx_material_map specular_factor;
			ufbx_material_map specular_color;
			ufbx_material_map specular_exponent;
			ufbx_material_map reflection_factor;
			ufbx_material_map reflection_color;
			ufbx_material_map transparency_factor;
			ufbx_material_map transparency_color;
			ufbx_material_map emission_factor;
			ufbx_material_map emission_color;
			ufbx_material_map ambient_factor;
			ufbx_material_map ambient_color;
			ufbx_material_map normal_map;
			ufbx_material_map bump;
			ufbx_material_map bump_factor;
			ufbx_material_map displacement_factor;
			ufbx_material_map displacement;
			ufbx_material_map vector_displacement_factor;
			ufbx_material_map vector_displacement;
		};
	};
} ufbx_material_fbx_maps;

typedef struct ufbx_material_pbr_maps {
	union {
		ufbx_material_map maps[UFBX_MATERIAL_PBR_MAP_COUNT];
		struct {
			ufbx_material_map base_factor;
			ufbx_material_map base_color;
			ufbx_material_map roughness;
			ufbx_material_map metalness;
			ufbx_material_map diffuse_roughness;
			ufbx_material_map specular_factor;
			ufbx_material_map specular_color;
			ufbx_material_map specular_ior;
			ufbx_material_map specular_anisotropy;
			ufbx_material_map specular_rotation;
			ufbx_material_map transmission_factor;
			ufbx_material_map transmission_color;
			ufbx_material_map transmission_depth;
			ufbx_material_map transmission_scatter;
			ufbx_material_map transmission_scatter_anisotropy;
			ufbx_material_map transmission_dispersion;
			ufbx_material_map transmission_roughness;
			ufbx_material_map transmission_extra_roughness;
			ufbx_material_map transmission_priority;
			ufbx_material_map transmission_enable_in_aov;
			ufbx_material_map subsurface_factor;
			ufbx_material_map subsurface_color;
			ufbx_material_map subsurface_radius;
			ufbx_material_map subsurface_scale;
			ufbx_material_map subsurface_anisotropy;
			ufbx_material_map subsurface_tint_color;
			ufbx_material_map subsurface_type;
			ufbx_material_map sheen_factor;
			ufbx_material_map sheen_color;
			ufbx_material_map sheen_roughness;
			ufbx_material_map coat_factor;
			ufbx_material_map coat_color;
			ufbx_material_map coat_roughness;
			ufbx_material_map coat_ior;
			ufbx_material_map coat_anisotropy;
			ufbx_material_map coat_rotation;
			ufbx_material_map coat_normal;
			ufbx_material_map coat_affect_base_color;
			ufbx_material_map coat_affect_base_roughness;
			ufbx_material_map thin_film_thickness;
			ufbx_material_map thin_film_ior;
			ufbx_material_map emission_factor;
			ufbx_material_map emission_color;
			ufbx_material_map opacity;
			ufbx_material_map indirect_diffuse;
			ufbx_material_map indirect_specular;
			ufbx_material_map normal_map;
			ufbx_material_map tangent_map;
			ufbx_material_map displacement_map;
			ufbx_material_map matte_factor;
			ufbx_material_map matte_color;
			ufbx_material_map ambient_occlusion;
		};
	};
} ufbx_material_pbr_maps;

typedef struct ufbx_material_features {
	union {
		ufbx_material_feature_info features[UFBX_MATERIAL_FEATURE_COUNT];
		struct {
			ufbx_material_feature_info metalness;
			ufbx_material_feature_info diffuse;
			ufbx_material_feature_info specular;
			ufbx_material_feature_info emission;
			ufbx_material_feature_info transmission;
			ufbx_material_feature_info coat;
			ufbx_material_feature_info sheen;
			ufbx_material_feature_info opacity;
			ufbx_material_feature_info ambient_occlusion;
			ufbx_material_feature_info matte;
			ufbx_material_feature_info unlit;
			ufbx_material_feature_info ior;
			ufbx_material_feature_info diffuse_roughness;
			ufbx_material_feature_info transmission_roughness;
			ufbx_material_feature_info thin_walled;
			ufbx_material_feature_info caustics;
			ufbx_material_feature_info exit_to_background;
			ufbx_material_feature_info internal_reflections;
			ufbx_material_feature_info double_sided;
		};
	};
} ufbx_material_features;

// Surface material properties such as color, roughness, etc. Each property may
// be optionally bound to an `ufbx_texture`.
struct ufbx_material {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// FBX builtin properties
	// NOTE: These may be empty if the material is using a custom shader
	ufbx_material_fbx_maps fbx;

	// PBR material properties, defined for all shading models but may be
	// somewhat approximate if `shader == NULL`.
	ufbx_material_pbr_maps pbr;

	// Material features, primarily applies to `pbr`.
	ufbx_material_features features;

	// Shading information
	ufbx_shader_type shader_type;      // < Always defined
	ufbx_nullable ufbx_shader *shader; // < Optional extended shader information
	ufbx_string shading_model_name;    // < Often one of `{ "lambert", "phong", "unknown" }`

	// Prefix before shader property names with trailing `|`.
	// For example `"3dsMax|Parameters|"` where properties would have names like
	// `"3dsMax|Parameters|base_color"`. You can ignore this if you use the built-in
	// `ufbx_material_fbx_maps fbx` and `ufbx_material_pbr_maps pbr` structures.
	ufbx_string shader_prop_prefix;    

	// All textures attached to the material, if you want specific maps if might be
	// more convenient to use eg. `fbx.diffuse_color.texture` or `pbr.base_color.texture`
	ufbx_material_texture_list textures; // < Sorted by `material_prop`
};

typedef enum ufbx_texture_type {

	// Texture associated with an image file/sequence. `texture->filename` and
	// and `texture->relative_filename` contain the texture's path. If the file
	// has embedded content `texture->content` may hold `texture->content_size`
	// bytes of raw image data.
	UFBX_TEXTURE_FILE,

	// The texture consists of multiple texture layers blended together.
	UFBX_TEXTURE_LAYERED,

	// Reserved as these _should_ exist in FBX files.
	UFBX_TEXTURE_PROCEDURAL,

	// Node in a shader graph.
	// Use `ufbx_texture.shader` for more information.
	UFBX_TEXTURE_SHADER,

	UFBX_TEXTURE_TYPE_COUNT,
	UFBX_TEXTURE_TYPE_FORCE_32BIT = 0x7fffffff,
} ufbx_texture_type;

// Blend modes to combine layered textures with, compatible with common blend
// mode definitions in many art programs. Simpler blend modes have equations
// specified below where `src` is the layer to compososite over `dst`.
// See eg. https://www.w3.org/TR/2013/WD-compositing-1-20131010/#blendingseparable
typedef enum ufbx_blend_mode {
	UFBX_BLEND_TRANSLUCENT,   // < `src` effects result alpha
	UFBX_BLEND_ADDITIVE,      // < `src + dst`
	UFBX_BLEND_MULTIPLY,      // < `src * dst`
	UFBX_BLEND_MULTIPLY_2X,   // < `2 * src * dst`
	UFBX_BLEND_OVER,          // < `src * src_alpha + dst * (1-src_alpha)`
	UFBX_BLEND_REPLACE,       // < `src` Replace the contents
	UFBX_BLEND_DISSOLVE,      // < `random() + src_alpha >= 1.0 ? src : dst`
	UFBX_BLEND_DARKEN,        // < `min(src, dst)`
	UFBX_BLEND_COLOR_BURN,    // < `src > 0 ? 1 - min(1, (1-dst) / src) : 0`
	UFBX_BLEND_LINEAR_BURN,   // < `src + dst - 1`
	UFBX_BLEND_DARKER_COLOR,  // < `value(src) < value(dst) ? src : dst`
	UFBX_BLEND_LIGHTEN,       // < `max(src, dst)`
	UFBX_BLEND_SCREEN,        // < `1 - (1-src)*(1-dst)`
	UFBX_BLEND_COLOR_DODGE,   // < `src < 1 ? dst / (1 - src)` : (dst>0?1:0)`
	UFBX_BLEND_LINEAR_DODGE,  // < `src + dst`
	UFBX_BLEND_LIGHTER_COLOR, // < `value(src) > value(dst) ? src : dst`
	UFBX_BLEND_SOFT_LIGHT,    // < https://www.w3.org/TR/2013/WD-compositing-1-20131010/#blendingsoftlight
	UFBX_BLEND_HARD_LIGHT,    // < https://www.w3.org/TR/2013/WD-compositing-1-20131010/#blendinghardlight
	UFBX_BLEND_VIVID_LIGHT,   // < Combination of `COLOR_DODGE` and `COLOR_BURN`
	UFBX_BLEND_LINEAR_LIGHT,  // < Combination of `LINEAR_DODGE` and `LINEAR_BURN`
	UFBX_BLEND_PIN_LIGHT,     // < Combination of `DARKEN` and `LIGHTEN`
	UFBX_BLEND_HARD_MIX,      // < Produces primary colors depending on similarity
	UFBX_BLEND_DIFFERENCE,    // < `abs(src - dst)`
	UFBX_BLEND_EXCLUSION,     // < `dst + src - 2 * src * dst`
	UFBX_BLEND_SUBTRACT,      // < `dst - src`
	UFBX_BLEND_DIVIDE,        // < `dst / src`
	UFBX_BLEND_HUE,           // < Replace hue
	UFBX_BLEND_SATURATION,    // < Replace saturation
	UFBX_BLEND_COLOR,         // < Replace hue and saturatio 
	UFBX_BLEND_LUMINOSITY,    // < Replace value
	UFBX_BLEND_OVERLAY,       // < Same as `HARD_LIGHT` but with `src` and `dst` swapped

	UFBX_BLEND_MODE_COUNT,
	UFBX_BLEND_MODE_FORCE_32BIT = 0x7fffffff,
} ufbx_blend_mode;

// Blend modes to combine layered textures with, compatible with common blend
typedef enum ufbx_wrap_mode {
	UFBX_WRAP_REPEAT, // < Repeat the texture past the [0,1] range
	UFBX_WRAP_CLAMP,  // < Clamp the normalized texture coordinates to [0,1]

	UFBX_WRAP_MODE_COUNT,
	UFBX_WRAP_MODE_FORCE_32BIT = 0x7fffffff,
} ufbx_wrap_mode;

// Single layer in a layered texture
typedef struct ufbx_texture_layer {
	ufbx_texture *texture;      // < The inner texture to evaluate, never `NULL`
	ufbx_blend_mode blend_mode; // < Equation to combine the layer to the background
	ufbx_real alpha;            // < Blend weight of this layer
} ufbx_texture_layer;

UFBX_LIST_TYPE(ufbx_texture_layer_list, ufbx_texture_layer);

typedef enum ufbx_shader_texture_type {
	UFBX_SHADER_TEXTURE_UNKNOWN,

	// Select an output of a multi-output shader.
	// HINT: If this type is used the `ufbx_shader_texture_texture` and
	// `ufbx_shader_texture.main_texture_output_index` fields are set.
	UFBX_SHADER_TEXTURE_SELECT_OUTPUT,

	// Open Shading Language (OSL) shader.
	// https://github.com/AcademySoftwareFoundation/OpenShadingLanguage
	UFBX_SHADER_TEXTURE_OSL,

	UFBX_SHADER_TEXTURE_TYPE_COUNT,
	UFBX_SHADER_TEXTURE_TYPE_FORCE_32BIT = 0x7fffffff,
} ufbx_shader_texture_type;

typedef struct ufbx_shader_texture_input {

	// Name of the input.
	ufbx_string name;

	// Constant value of the input.
	union {
		ufbx_real value_real;
		ufbx_vec2 value_vec2;
		ufbx_vec3 value_vec3;
		ufbx_vec4 value_vec4;
	};
	int64_t value_int;
	ufbx_string value_str;
	ufbx_blob value_blob;

	// Texture connected to this input.
	ufbx_nullable ufbx_texture *texture;

	// Index of the output to use if `texture` is a multi-output shader node.
	int64_t texture_output_index;

	// Controls whether shading should use `texture`.
	// NOTE: Some shading models allow this to be `true` even if `texture == NULL`.
	bool texture_enabled;

	// Property representing this input.
	ufbx_prop *prop;

	// Property representing `texture`.
	ufbx_nullable ufbx_prop *texture_prop;

	// Property representing `texture_enabled`.
	ufbx_nullable ufbx_prop *texture_enabled_prop;

} ufbx_shader_texture_input;

UFBX_LIST_TYPE(ufbx_shader_texture_input_list, ufbx_shader_texture_input);

typedef struct ufbx_shader_texture {

	// Type of this shader node.
	ufbx_shader_texture_type type;

	// Name of the shader to use.
	ufbx_string shader_name;

	// 64-bit opaque identifier for the shader type.
	uint64_t shader_type_id;

	// Input values/textures (possibly further shader textures) to the shader.
	// Sorted by `ufbx_shader_texture_input.name`.
	ufbx_shader_texture_input_list inputs;

	// Shader source code if found.
	ufbx_string shader_source;
	ufbx_blob raw_shader_source;

	// Representative texture for this shader.
	// Only specified if `main_texture.outputs[main_texture_output_index]` is semantically
	// equivalent to this texture.
	ufbx_texture *main_texture;

	// Output index of `main_texture` if it is a multi-output shader.
	int64_t main_texture_output_index;

	// Prefix for properties related to this shader in `ufbx_texture`.
	// NOTE: Contains the trailing '|' if not empty.
	ufbx_string prop_prefix;

} ufbx_shader_texture;

// Texture that controls material appearance
struct ufbx_texture {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// Texture type (file / layered / procedural / shader)
	ufbx_texture_type type;

	// FILE: Paths to the resource
	ufbx_string filename;
	ufbx_string absolute_filename;
	ufbx_string relative_filename;
	ufbx_blob raw_filename;
	ufbx_blob raw_absolute_filename;
	ufbx_blob raw_relative_filename;

	// FILE: Optional embedded content blob, eg. raw .png format data
	ufbx_blob content;

	// FILE: Optional video texture
	ufbx_nullable ufbx_video *video;

	// LAYERED: Inner texture layers, ordered from _bottom_ to _top_
	ufbx_texture_layer_list layers;

	// SHADER: Shader information
	// NOTE: May be specified even if `type == UFBX_TEXTURE_FILE` if `ufbx_load_opts.disable_quirks`
	// is _not_ specified. Some known shaders that represent files are interpreted as `UFBX_TEXTURE_FILE`.
	ufbx_nullable ufbx_shader_texture *shader;

	// List of file textures representing this texture.
	// Defined even if `type == UFBX_TEXTURE_FILE` in which case the array contains only itself.
	ufbx_texture_list file_textures;

	// Name of the UV set to use
	ufbx_string uv_set;

	// Wrapping mode
	ufbx_wrap_mode wrap_u;
	ufbx_wrap_mode wrap_v;

	// UV transform
	ufbx_transform transform;  // < Texture transformation in UV space
	ufbx_matrix texture_to_uv; // < Matrix representation of `transform`
	ufbx_matrix uv_to_texture; // < UV coordinate to normalized texture coordinate matrix
};

// TODO: Video textures
struct ufbx_video {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// Paths to the resource
	ufbx_string filename;
	ufbx_string absolute_filename;
	ufbx_string relative_filename;
	ufbx_blob raw_filename;
	ufbx_blob raw_absolute_filename;
	ufbx_blob raw_relative_filename;

	// Optional embedded content blob
	ufbx_blob content;
};

// Shader specifies a shading model and contains `ufbx_shader_binding` elements
// that define how to interpret FBX properties in the shader.
struct ufbx_shader {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// Known shading model
	ufbx_shader_type type;

	// TODO: Expose actual properties here

	// Bindings from FBX properties to the shader
	// HINT: `ufbx_find_shader_prop()` translates shader properties to FBX properties
	ufbx_shader_binding_list bindings;
};

// Binding from a material property to shader implementation
typedef struct ufbx_shader_prop_binding {
	ufbx_string shader_prop;   // < Property name used by the shader implementation
	ufbx_string material_prop; // < Property name inside `ufbx_material.props`
} ufbx_shader_prop_binding;

UFBX_LIST_TYPE(ufbx_shader_prop_binding_list, ufbx_shader_prop_binding);

// Shader binding table
struct ufbx_shader_binding {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	ufbx_shader_prop_binding_list prop_bindings; // < Sorted by `shader_prop`
};

// -- Animation

typedef struct ufbx_anim_layer_desc {
	ufbx_anim_layer *layer;
	ufbx_real weight;
} ufbx_anim_layer_desc;

UFBX_LIST_TYPE(ufbx_const_anim_layer_desc_list, const ufbx_anim_layer_desc);

typedef struct ufbx_prop_override {
	// Element (`ufbx_element.id`) to override the property from
	// NOTE: You can get this from typed structs eg. `my_node->element.id`
	uint32_t element_id;

	// Property name to override.
	const char *prop_name;

	// Override value, use `value.x` for scalars. `value_int` is initialized
	// from `value.x` if zero so keep `value` zeroed even if you don't need it!
	ufbx_vec3 value;
	const char *value_str;
	int64_t value_int;

	// Internal: Gets filled automatically by `ufbx_prepare_prop_overrides()`
	uint32_t _internal_key;
} ufbx_prop_override;

UFBX_LIST_TYPE(ufbx_const_prop_override_list, const ufbx_prop_override);

typedef struct ufbx_anim {
	ufbx_const_anim_layer_desc_list layers;

	// Override individual `ufbx_prop` values from elements
	// NOTE: Call `ufbx_prepare_prop_overrides()` to obtain this!
	ufbx_const_prop_override_list prop_overrides;

	bool ignore_connections;

	// Not used by evaluation
	double time_begin;
	double time_end;
} ufbx_anim;

struct ufbx_anim_stack {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	double time_begin;
	double time_end;

	ufbx_anim_layer_list layers;
	ufbx_anim anim;
};

typedef struct ufbx_anim_prop {
	ufbx_element *element;
	uint32_t _internal_key;
	ufbx_string prop_name;
	ufbx_anim_value *anim_value;
} ufbx_anim_prop;

UFBX_LIST_TYPE(ufbx_anim_prop_list, ufbx_anim_prop);

struct ufbx_anim_layer {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	ufbx_real weight;
	bool weight_is_animated;
	bool blended;
	bool additive;
	bool compose_rotation;
	bool compose_scale;

	ufbx_anim_value_list anim_values;
	ufbx_anim_prop_list anim_props; // < Sorted by `element,prop_name`

	ufbx_anim anim;

	uint32_t _min_element_id;
	uint32_t _max_element_id;
	uint32_t _element_id_bitmask[4];
};

struct ufbx_anim_value {
	union { ufbx_element element; struct { ufbx_string name; ufbx_props props; }; };

	ufbx_vec3 default_value;
	ufbx_nullable ufbx_anim_curve *curves[3];
};

// Animation curve segment interpolation mode between two keyframes
typedef enum ufbx_interpolation {
	UFBX_INTERPOLATION_CONSTANT_PREV, // < Hold previous key value
	UFBX_INTERPOLATION_CONSTANT_NEXT, // < Hold next key value
	UFBX_INTERPOLATION_LINEAR,        // < Linear interpolation between two keys
	UFBX_INTERPOLATION_CUBIC,         // < Cubic interpolation, see `ufbx_tangent`

	UFBX_INTERPOLATION_COUNT,
	UFBX_INTERPOLATION_FORCE_32BIT = 0x7fffffff,
} ufbx_interpolation;

// Tangent vector at a keyframe, may be split into left/right
typedef struct ufbx_tangent {
	float dx; // < Derivative in the time axis
	float dy; // < Derivative in the (curve specific) value axis
} ufbx_tangent;

// Single real `value` at a specified `time`, interpolation between two keyframes
// is determined by the `interpolation` field of the _previous_ key.
// If `interpolation == UFBX_INTERPOLATION_CUBIC` the span is evaluated as a
// cubic bezier curve through the following points:
//
//   (prev->time, prev->value)
//   (prev->time + prev->right.dx, prev->value + prev->right.dy)
//   (next->time - next->left.dx, next->value - next->left.dy)
//   (next->time, next->value)
//
// HINT: You can use `ufbx_evaluate_curve(ufbx_anim_curve *curve, double time)`
// rather than trying to manually handle all the interpolation modes.
typedef struct ufbx_keyframe {
	double time;
	ufbx_real value;
	ufbx_interpolation interpolation;
	ufbx_tangent left;
	ufbx_tangent right;
} ufbx_keyframe;

UFBX_LIST_TYPE(ufbx_keyframe_list, ufbx_keyframe);

struct ufbx_anim_curve {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	ufbx_keyframe_list keyframes;
};

// -- Collections

// Collection of nodes to hide/freeze
struct ufbx_display_layer {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// Nodes included in the layer (exclusively at most one layer per node)
	ufbx_node_list nodes;

	// Layer state
	bool visible; // < Contained nodes are visible
	bool frozen;  // < Contained nodes cannot be edited

	ufbx_vec3 ui_color; // < Visual color for UI
};

// Named set of nodes/geometry features to select.
struct ufbx_selection_set {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// Included nodes and geomtery features
	ufbx_selection_node_list nodes;
};

// Selection state of a node, potentially contains vertex/edge/face selection as well.
struct ufbx_selection_node {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// Selection targets, possibly `NULL`
	ufbx_nullable ufbx_node *target_node;
	ufbx_nullable ufbx_mesh *target_mesh;
	bool include_node; // < Is `target_node` included in the selection

	// Indices to selected components.
	// Guaranteed to be valid as per `ufbx_load_opts.index_error_handling`
	// if `target_mesh` is not `NULL`.
	ufbx_uint32_list vertices; // < Indices to `ufbx_mesh.vertices`
	ufbx_uint32_list edges;    // < Indices to `ufbx_mesh.edges`
	ufbx_uint32_list faces;    // < Indices to `ufbx_mesh.faces`
};

// -- Constraints

struct ufbx_character {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };
};

// Type of property constrain eg. position or look-at
typedef enum ufbx_constraint_type {
	UFBX_CONSTRAINT_UNKNOWN,
	UFBX_CONSTRAINT_AIM,
	UFBX_CONSTRAINT_PARENT,
	UFBX_CONSTRAINT_POSITION,
	UFBX_CONSTRAINT_ROTATION,
	UFBX_CONSTRAINT_SCALE,
	// Inverse kinematic chain to a single effector `ufbx_constraint.ik_effector`
	// `targets` optionally contains a list of pole targets!
	UFBX_CONSTRAINT_SINGLE_CHAIN_IK,

	UFBX_CONSTRAINT_TYPE_COUNT,
	UFBX_CONSTRAINT_TYPE_FORCE_32BIT = 0x7fffffff,
} ufbx_constraint_type;

// Target to follow with a constraint
typedef struct ufbx_constraint_target {
	ufbx_node *node;          // < Target node reference
	ufbx_real weight;         // < Relative weight to other targets (does not always sum to 1)
	ufbx_transform transform; // < Offset from the actual target
} ufbx_constraint_target;

UFBX_LIST_TYPE(ufbx_constraint_target_list, ufbx_constraint_target);

// Method to determine the up vector in aim constraints
typedef enum ufbx_constraint_aim_up_type {
	UFBX_CONSTRAINT_AIM_UP_SCENE,      // < Align the up vector to the scene global up vector
	UFBX_CONSTRAINT_AIM_UP_TO_NODE,    // < Aim the up vector at `ufbx_constraint.aim_up_node`
	UFBX_CONSTRAINT_AIM_UP_ALIGN_NODE, // < Copy the up vector from `ufbx_constraint.aim_up_node`
	UFBX_CONSTRAINT_AIM_UP_VECTOR,     // < Use `ufbx_constraint.aim_up_vector` as the up vector
	UFBX_CONSTRAINT_AIM_UP_NONE,       // < Don't align the up vector to anything

	UFBX_CONSTRAINT_AIM_UP_TYPE_COUNT,
	UFBX_CONSTRAINT_AIM_UP_TYPE_FORCE_32BIT = 0x7fffffff,
} ufbx_constraint_aim_up_type;

// Method to determine the up vector in aim constraints
typedef enum ufbx_constraint_ik_pole_type {
	UFBX_CONSTRAINT_IK_POLE_VECTOR, // < Use towards calculated from `ufbx_constraint.targets`
	UFBX_CONSTRAINT_IK_POLE_NODE,   // < Use `ufbx_constraint.ik_pole_vector` directly

	UFBX_CONSTRAINT_IK_POLE_TYPE_COUNT,
	UFBX_CONSTRAINT_IK_POLE_TYPE_FORCE_32BIT = 0x7fffffff,
} ufbx_constraint_ik_pole_type;

struct ufbx_constraint {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	// Type of constraint to use
	ufbx_constraint_type type;
	ufbx_string type_name;

	// Node to be constrained
	ufbx_nullable ufbx_node *node;

	// List of weighted targets for the constraint (pole vectors for IK)
	ufbx_constraint_target_list targets;

	// State of the constraint
	ufbx_real weight;
	bool active;

	// Translation/rotation/scale axes the constraint is applied to
	bool constrain_translation[3];
	bool constrain_rotation[3];
	bool constrain_scale[3];

	// Offset from the constrained position
	ufbx_transform transform_offset;

	// AIM: Target and up vectors
	ufbx_vec3 aim_vector;
	ufbx_constraint_aim_up_type aim_up_type;
	ufbx_nullable ufbx_node *aim_up_node;
	ufbx_vec3 aim_up_vector;

	// SINGLE_CHAIN_IK: Target for the IK, `targets` contains pole vectors!
	ufbx_nullable ufbx_node *ik_effector;
	ufbx_nullable ufbx_node *ik_end_node;
	ufbx_vec3 ik_pole_vector;
};

// -- Miscellaneous

typedef struct ufbx_bone_pose {
	ufbx_node *bone_node;
	ufbx_matrix bone_to_world;
} ufbx_bone_pose;

UFBX_LIST_TYPE(ufbx_bone_pose_list, ufbx_bone_pose);

struct ufbx_pose {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };

	bool bind_pose;
	ufbx_bone_pose_list bone_poses;
};

struct ufbx_metadata_object {
	union { ufbx_element element; struct {
		ufbx_string name;
		ufbx_props props;
		uint32_t element_id;
		uint32_t typed_id;
	}; };
};

// -- Named elements

typedef struct ufbx_name_element {
	ufbx_string name;
	ufbx_element_type type;
	uint32_t _internal_key;
	ufbx_element *element;
} ufbx_name_element;

UFBX_LIST_TYPE(ufbx_name_element_list, ufbx_name_element);

// -- Scene

// Scene is the root object loaded by ufbx that everything is accessed from.

typedef enum ufbx_exporter {
	UFBX_EXPORTER_UNKNOWN,
	UFBX_EXPORTER_FBX_SDK,
	UFBX_EXPORTER_BLENDER_BINARY,
	UFBX_EXPORTER_BLENDER_ASCII,
	UFBX_EXPORTER_MOTION_BUILDER,
	UFBX_EXPORTER_BC_UNITY_EXPORTER,

	UFBX_EXPORTER_COUNT,
	UFBX_EXPORTER_FORCE_32BIT = 0x7fffffff,
} ufbx_exporter;

typedef struct ufbx_application {
	ufbx_string vendor;
	ufbx_string name;
	ufbx_string version;
} ufbx_application;

// Miscellaneous data related to the loaded file
typedef struct ufbx_metadata {

	// FBX ASCII file format.
	bool ascii;

	// FBX version in integer format, eg. 7400 for 7.4.
	uint32_t version;

	// Index arrays may contain `UFBX_NO_INDEX` instead of a valid index
	// to indicate gaps.
	bool may_contain_no_index;

	// Some API guarantees do not apply (depending on unsafe options used).
	// Loaded with `ufbx_load_opts.allow_unsafe` enabled.
	bool unsafe;

	ufbx_string creator;
	bool big_endian;

	ufbx_string filename;
	ufbx_string relative_root;

	ufbx_blob raw_filename;
	ufbx_blob raw_relative_root;

	ufbx_exporter exporter;
	uint32_t exporter_version;

	ufbx_props scene_props;

	ufbx_application original_application;
	ufbx_application latest_application;

	bool geometry_ignored;
	bool animation_ignored;
	bool embedded_ignored;

	size_t max_face_triangles;

	size_t result_memory_used;
	size_t temp_memory_used;
	size_t result_allocs;
	size_t temp_allocs;

	size_t element_buffer_size;
	size_t num_shader_textures;

	ufbx_real bone_prop_size_unit;
	bool bone_prop_limb_length_relative;
	double ktime_to_sec;

	ufbx_string original_file_path;
	ufbx_blob raw_original_file_path;

} ufbx_metadata;

typedef enum ufbx_coordinate_axis {
	UFBX_COORDINATE_AXIS_POSITIVE_X,
	UFBX_COORDINATE_AXIS_NEGATIVE_X,
	UFBX_COORDINATE_AXIS_POSITIVE_Y,
	UFBX_COORDINATE_AXIS_NEGATIVE_Y,
	UFBX_COORDINATE_AXIS_POSITIVE_Z,
	UFBX_COORDINATE_AXIS_NEGATIVE_Z,
	UFBX_COORDINATE_AXIS_UNKNOWN,

	UFBX_COORDINATE_AXIS_COUNT,
	UFBX_COORDINATE_AXIS_FORCE_32BIT = 0x7fffffff,
} ufbx_coordinate_axis;

// Coordinate axes the scene is represented in.
// NOTE: `front` is the _opposite_ from forward!
typedef struct ufbx_coordinate_axes {
	ufbx_coordinate_axis right;
	ufbx_coordinate_axis up;
	ufbx_coordinate_axis front;
} ufbx_coordinate_axes;

typedef enum ufbx_time_mode {
	UFBX_TIME_MODE_DEFAULT,
	UFBX_TIME_MODE_120_FPS,
	UFBX_TIME_MODE_100_FPS,
	UFBX_TIME_MODE_60_FPS,
	UFBX_TIME_MODE_50_FPS,
	UFBX_TIME_MODE_48_FPS,
	UFBX_TIME_MODE_30_FPS,
	UFBX_TIME_MODE_30_FPS_DROP,
	UFBX_TIME_MODE_NTSC_DROP_FRAME,
	UFBX_TIME_MODE_NTSC_FULL_FRAME,
	UFBX_TIME_MODE_PAL,
	UFBX_TIME_MODE_24_FPS,
	UFBX_TIME_MODE_1000_FPS,
	UFBX_TIME_MODE_FILM_FULL_FRAME,
	UFBX_TIME_MODE_CUSTOM,
	UFBX_TIME_MODE_96_FPS,
	UFBX_TIME_MODE_72_FPS,
	UFBX_TIME_MODE_59_94_FPS,

	UFBX_TIME_MODE_COUNT,
	UFBX_TIME_MODE_FORCE_32BIT = 0x7fffffff,
} ufbx_time_mode;

typedef enum ufbx_time_protocol {
	UFBX_TIME_PROTOCOL_SMPTE,
	UFBX_TIME_PROTOCOL_FRAME_COUNT,
	UFBX_TIME_PROTOCOL_DEFAULT,

	UFBX_TIME_PROTOCOL_COUNT,
	UFBX_TIME_PROTOCOL_FORCE_32BIT = 0x7fffffff,
} ufbx_time_protocol;

typedef enum ufbx_snap_mode {
	UFBX_SNAP_MODE_NONE,
	UFBX_SNAP_MODE_SNAP,
	UFBX_SNAP_MODE_PLAY,
	UFBX_SNAP_MODE_SNAP_AND_PLAY,

	UFBX_SNAP_MODE_COUNT,
	UFBX_SNAP_MODE_FORCE_32BIT = 0x7fffffff,
} ufbx_snap_mode;

// Global settings: Axes and time/unit scales
typedef struct ufbx_scene_settings {
	ufbx_props props;

	// Mapping of X/Y/Z axes to world-space directions.
	// HINT: Use `ufbx_load_opts.target_axes` to normalize this.
	// NOTE: This contains the _original_ axes even if you supply `ufbx_load_opts.target_axes`.
	ufbx_coordinate_axes axes;

	// How many meters does a single world-space unit represent.
	// FBX files usually default to centimeters, reported as `0.01` here.
	// HINT: Use `ufbx_load_opts.target_unit_meters` to normalize this.
	ufbx_real unit_meters;

	double frames_per_second;

	ufbx_vec3 ambient_color;
	ufbx_string default_camera;

	ufbx_time_mode time_mode;
	ufbx_time_protocol time_protocol;
	ufbx_snap_mode snap_mode;

	// Original settings (?)
	ufbx_coordinate_axis original_axis_up;
	ufbx_real original_unit_meters;
} ufbx_scene_settings;

struct ufbx_scene {
	ufbx_metadata metadata;

	// Global settings
	ufbx_scene_settings settings;

	// Node instances in the scene
	ufbx_node *root_node;

	// Default animation descriptor
	ufbx_anim anim;

	// All animation stacks combined
	ufbx_anim combined_anim;

	union {
		struct {
			ufbx_unknown_list unknowns;

			// Nodes
			ufbx_node_list nodes;

			// Node attributes (common)
			ufbx_mesh_list meshes;
			ufbx_light_list lights;
			ufbx_camera_list cameras;
			ufbx_bone_list bones;
			ufbx_empty_list empties;

			// Node attributes (curves/surfaces)
			ufbx_line_curve_list line_curves;
			ufbx_nurbs_curve_list nurbs_curves;
			ufbx_nurbs_surface_list nurbs_surfaces;
			ufbx_nurbs_trim_surface_list nurbs_trim_surfaces;
			ufbx_nurbs_trim_boundary_list nurbs_trim_boundaries;

			// Node attributes (advanced)
			ufbx_procedural_geometry_list procedural_geometries;
			ufbx_stereo_camera_list stereo_cameras;
			ufbx_camera_switcher_list camera_switchers;
			ufbx_marker_list markers;
			ufbx_lod_group_list lod_groups;

			// Deformers
			ufbx_skin_deformer_list skin_deformers;
			ufbx_skin_cluster_list skin_clusters;
			ufbx_blend_deformer_list blend_deformers;
			ufbx_blend_channel_list blend_channels;
			ufbx_blend_shape_list blend_shapes;
			ufbx_cache_deformer_list cache_deformers;
			ufbx_cache_file_list cache_files;

			// Materials
			ufbx_material_list materials;
			ufbx_texture_list textures;
			ufbx_video_list videos;
			ufbx_shader_list shaders;
			ufbx_shader_binding_list shader_bindings;

			// Animation
			ufbx_anim_stack_list anim_stacks;
			ufbx_anim_layer_list anim_layers;
			ufbx_anim_value_list anim_values;
			ufbx_anim_curve_list anim_curves;

			// Collections
			ufbx_display_layer_list display_layers;
			ufbx_selection_set_list selection_sets;
			ufbx_selection_node_list selection_nodes;

			// Constraints
			ufbx_character_list characters;
			ufbx_constraint_list constraints;

			// Miscellaneous
			ufbx_pose_list poses;
			ufbx_metadata_object_list metadata_objects;
		};

		ufbx_element_list elements_by_type[UFBX_ELEMENT_TYPE_COUNT];
	};

	// All elements and connections in the whole file
	ufbx_element_list elements;           // < Sorted by `id`
	ufbx_connection_list connections_src; // < Sorted by `src,src_prop`
	ufbx_connection_list connections_dst; // < Sorted by `dst,dst_prop`

	// Elements sorted by name, type
	ufbx_name_element_list elements_by_name;

	// Enabled if `ufbx_load_opts.retain_dom == true`.
	ufbx_nullable ufbx_dom_node *dom_root;
};

// -- Curves

typedef struct ufbx_curve_point {
	bool valid;
	ufbx_vec3 position;
	ufbx_vec3 derivative;
} ufbx_curve_point;

typedef struct ufbx_surface_point {
	bool valid;
	ufbx_vec3 position;
	ufbx_vec3 derivative_u;
	ufbx_vec3 derivative_v;
} ufbx_surface_point;

// -- Mesh topology

typedef enum ufbx_topo_flags {
	UFBX_TOPO_NON_MANIFOLD = 0x1, // < Edge with three or more faces

	UFBX_TOPO_FLAGS_FORCE_32BIT = 0x7fffffff,
} ufbx_topo_flags;

typedef struct ufbx_topo_edge {
	uint32_t index; // < Starting index of the edge, always defined
	uint32_t next;  // < Ending index of the edge / next per-face `ufbx_topo_edge`, always defined
	uint32_t prev;  // < Previous per-face `ufbx_topo_edge`, always defined
	uint32_t twin;  // < `ufbx_topo_edge` on the opposite side, `UFBX_NO_INDEX` if not found
	uint32_t face;  // < Index into `mesh->faces[]`, always defined
	uint32_t edge;  // < Index into `mesh->edges[]`, `UFBX_NO_INDEX` if not found

	ufbx_topo_flags flags;
} ufbx_topo_edge;

typedef struct ufbx_vertex_stream {
	const void *data;
	size_t vertex_size;
} ufbx_vertex_stream;

// -- Memory callbacks

// You can optionally provide an allocator to ufbx, the default is to use the
// CRT malloc/realloc/free

// Allocate `size` bytes, must be at least 8 byte aligned
typedef void *ufbx_alloc_fn(void *user, size_t size);

// Reallocate `old_ptr` from `old_size` to `new_size`
// NOTE: If omit `alloc_fn` and `free_fn` they will be translated to:
//   `alloc(size)` -> `realloc_fn(user, NULL, 0, size)`
//   `free_fn(ptr, size)` ->  `realloc_fn(user, ptr, size, 0)`
typedef void *ufbx_realloc_fn(void *user, void *old_ptr, size_t old_size, size_t new_size);

// Free pointer `ptr` (of `size` bytes) returned by `alloc_fn` or `realloc_fn`
typedef void ufbx_free_fn(void *user, void *ptr, size_t size);

// Free the allocator itself
typedef void ufbx_free_allocator_fn(void *user);

// Allocator callbacks and user context
// NOTE: The allocator will be stored to the loaded scene and will be called
// again from `ufbx_free_scene()` so make sure `user` outlives that!
// You can use `free_allocator_fn()` to free the allocator yourself.
typedef struct ufbx_allocator {
	// Callback functions, see `typedef`s above for information
	ufbx_alloc_fn *alloc_fn;
	ufbx_realloc_fn *realloc_fn;
	ufbx_free_fn *free_fn;
	ufbx_free_allocator_fn *free_allocator_fn;
	void *user;
} ufbx_allocator;

typedef struct ufbx_allocator_opts {
	// Allocator callbacks
	ufbx_allocator allocator;

	// Maximum number of bytes to allocate before failing
	size_t memory_limit;

	// Maximum number of allocations to attempt before failing
	size_t allocation_limit;

	// Threshold to swap from batched allocations to individual ones
	// Defaults to 1MB if set to zero
	// NOTE: If set to `1` ufbx will allocate everything in the smallest
	// possible chunks which may be useful for debugging (eg. ASAN)
	size_t huge_threshold;

	// Maximum size of a single allocation containing sub-allocations.
	// Defaults to 16MB if set to zero
	// The maximum amount of wasted memory depends on `max_chunk_size` and
	// `huge_threshold`: each chunk can waste up to `huge_threshold` bytes
	// internally and the last chunk might be incomplete. So for example
	// with the defaults we can waste around 1MB/16MB = 6.25% overall plus
	// up to 32MB due to the two incomplete blocks. The actual amounts differ
	// slightly as the chunks start out at 4kB and double in size each time,
	// meaning that the maximum fixed overhead (up to 32MB with defaults) is
	// at most ~30% of the total allocation size.
	size_t max_chunk_size;

} ufbx_allocator_opts;

// -- IO callbacks

// Try to read up to `size` bytes to `data`, return the amount of read bytes.
// Return `SIZE_MAX` to indicate an IO error.
typedef size_t ufbx_read_fn(void *user, void *data, size_t size);

// Skip `size` bytes in the file.
typedef bool ufbx_skip_fn(void *user, size_t size);

// Close the file
typedef void ufbx_close_fn(void *user);

typedef struct ufbx_stream {
	ufbx_read_fn *read_fn;   // < Required
	ufbx_skip_fn *skip_fn;   // < Optional: Will use `read_fn()` if missing
	ufbx_close_fn *close_fn; // < Optional

	// Context passed to other functions
	void *user;
} ufbx_stream;

// Callback for opening an external file from the filesystem
typedef bool ufbx_open_file_fn(void *user, ufbx_stream *stream, const char *path, size_t path_len);

typedef struct ufbx_open_file_cb { 
	ufbx_open_file_fn *fn;
	void *user;

	UFBX_CALLBACK_IMPL(ufbx_open_file_cb, ufbx_open_file_fn,
		(void *user, ufbx_stream *stream, const char *path, size_t path_len),
		(stream, path, path_len))
} ufbx_open_file_cb;

// Detailed error stack frame
typedef struct ufbx_error_frame {
	uint32_t source_line;
	ufbx_string function;
	ufbx_string description;
} ufbx_error_frame;

// Error causes (and `UFBX_ERROR_NONE` for no error).
typedef enum ufbx_error_type {

	// No error, operation has been performed succesfully.
	UFBX_ERROR_NONE,

	// Unspecified error, most likely caused by an invalid FBX file or a file
	// that contains something ufbx can't handle.
	UFBX_ERROR_UNKNOWN,

	// File not found.
	UFBX_ERROR_FILE_NOT_FOUND,

	// Out of memory (allocator returned `NULL`).
	UFBX_ERROR_OUT_OF_MEMORY,

	// `ufbx_allocator_opts.memory_limit` exhausted.
	UFBX_ERROR_MEMORY_LIMIT,

	// `ufbx_allocator_opts.allocation_limit` exhausted.
	UFBX_ERROR_ALLOCATION_LIMIT,

	// File ended abruptly.
	UFBX_ERROR_TRUNCATED_FILE,

	// IO read error.
	// eg. returning `SIZE_MAX` from `ufbx_stream.read_fn` or stdio `ferror()` condition.
	UFBX_ERROR_IO,

	// User cancelled the loading via `ufbx_load_opts.progress_cb` returning `UFBX_PROGRESS_CANCEL`.
	UFBX_ERROR_CANCELLED,

	// File is not an FBX file.
	UFBX_ERROR_NOT_FBX,

	// Options struct (eg. `ufbx_load_opts`) is not cleared to zero.
	// Make sure you initialize the structure to zero via eg.
	//   ufbx_load_opts opts = { 0 }; // C
	//   ufbx_load_opts opts = { }; // C++
	UFBX_ERROR_UNINITIALIZED_OPTIONS,

	// The vertex streams in `ufbx_generate_indices()` are empty.
	UFBX_ERROR_ZERO_VERTEX_SIZE,

	// Invalid UTF-8 encountered in a file when loading with `UFBX_UNICODE_ERROR_HANDLING_ABORT_LOADING`.
	UFBX_ERROR_INVALID_UTF8,

	// Feature needed for the operation has been compiled out.
	UFBX_ERROR_FEATURE_DISABLED,

	// Attempting to tessellate an invalid NURBS object.
	// See `ufbx_nurbs_basis.valid`.
	UFBX_ERROR_BAD_NURBS,

	// Out of bounds index in the file when loading with `UFBX_INDEX_ERROR_HANDLING_ABORT_LOADING`.
	UFBX_ERROR_BAD_INDEX,

	// Unsafe options specified without enabling `ufbx_load_opts.allow_unsafe`.
	UFBX_ERROR_UNSAFE_OPTIONS,

	UFBX_ERROR_TYPE_COUNT,
	UFBX_ERROR_TYPE_FORCE_32BIT = 0x7fffffff,
} ufbx_error_type;

// Error description with detailed stack trace
// HINT: You can use `ufbx_format_error()` for formatting the error
typedef struct ufbx_error {
	ufbx_error_type type;
	ufbx_string description;
	uint32_t stack_size;
	ufbx_error_frame stack[UFBX_ERROR_STACK_MAX_DEPTH];
} ufbx_error;

// -- Progress callbacks

typedef struct ufbx_progress {
	uint64_t bytes_read;
	uint64_t bytes_total;
} ufbx_progress;

typedef enum ufbx_progress_result {
	UFBX_PROGRESS_CONTINUE = 0x100,
	UFBX_PROGRESS_CANCEL = 0x200,

	UFBX_PROGRESS_FORCE_32BIT = 0x7fffffff,
} ufbx_progress_result;

// Called periodically with the current progress
// Return `false` to cancel further processing
typedef ufbx_progress_result ufbx_progress_fn(void *user, const ufbx_progress *progress);

typedef struct ufbx_progress_cb {
	ufbx_progress_fn *fn;
	void *user;

	UFBX_CALLBACK_IMPL(ufbx_progress_cb, ufbx_progress_fn,
		(void *user, const ufbx_progress *progress),
		(progress))
} ufbx_progress_cb;

// -- Inflate

typedef struct ufbx_inflate_input ufbx_inflate_input;
typedef struct ufbx_inflate_retain ufbx_inflate_retain;

// Source data/stream to decompress with `ufbx_inflate()`
struct ufbx_inflate_input {
	// Total size of the data in bytes
	size_t total_size;

	// (optional) Initial or complete data chunk
	const void *data;
	size_t data_size;

	// (optional) Temporary buffer, defaults to 256b stack buffer
	void *buffer;
	size_t buffer_size;

	// (optional) Streaming read function, concatenated after `data`
	ufbx_read_fn *read_fn;
	void *read_user;

	// (optional) Progress reporting
	ufbx_progress_cb progress_cb;
	uint64_t progress_interval_hint; // < Bytes between progress report calls

	// (optional) Change the progress scope
	uint64_t progress_size_before;
	uint64_t progress_size_after;

	// (optional) No the DEFLATE header
	bool no_header;

	// (optional) No the Adler32 checksum
	bool no_checksum;
};

// Persistent data between `ufbx_inflate()` calls
// NOTE: You must set `initialized` to `false`, but `data` may be uninitialized
struct ufbx_inflate_retain {
	bool initialized;
	uint64_t data[512];
};

typedef enum ufbx_index_error_handling {
	// Clamp to a valid value.
	UFBX_INDEX_ERROR_HANDLING_CLAMP,
	// Set bad indices to `UFBX_NO_INDEX`.
	// This is the recommended way if you need to deal with files with gaps in information.
	// HINT: If you use this `ufbx_get_vertex_TYPE()` functions will return zero
	// on invalid indices instead of failing.
	UFBX_INDEX_ERROR_HANDLING_NO_INDEX,
	// Fail loading entierely when encountering a bad index.
	UFBX_INDEX_ERROR_HANDLING_ABORT_LOADING,
	// Pass bad indices through as-is.
	// Requires `ufbx_load_opts.allow_unsafe`.
	// UNSAFE: Breaks any API guarantees regarding indexes being in bounds and makes
	// `ufbx_get_vertex_TYPE()` memory-unsafe to use.
	UFBX_INDEX_ERROR_HANDLING_UNSAFE_IGNORE,

	UFBX_INDEX_ERROR_HANDLING_COUNT,
	UFBX_INDEX_ERROR_HANDLING_FORCE_32BIT = 0x7fffffff,
} ufbx_index_error_handling;

typedef enum ufbx_unicode_error_handling {
	// Replace errors with U+FFFD "Replacement Character"
	UFBX_UNICODE_ERROR_HANDLING_REPLACEMENT_CHARACTER,
	// Replace errors with '_' U+5F "Low Line"
	UFBX_UNICODE_ERROR_HANDLING_UNDERSCORE,
	// Replace errors with '?' U+3F "Question Mark"
	UFBX_UNICODE_ERROR_HANDLING_QUESTION_MARK,
	// Remove errors from the output
	UFBX_UNICODE_ERROR_HANDLING_REMOVE,
	// Fail loading on encountering an Unicode error
	UFBX_UNICODE_ERROR_HANDLING_ABORT_LOADING,
	// Ignore and pass-through non-UTF-8 string data.
	// Requires `ufbx_load_opts.allow_unsafe`.
	// UNSAFE: Breaks API guarantee that `ufbx_string` is UTF-8 encoded.
	UFBX_UNICODE_ERROR_HANDLING_UNSAFE_IGNORE,

	UFBX_UNICODE_ERROR_HANDLING_COUNT,
	UFBX_UNICODE_ERROR_HANDLING_FORCE_32BIT = 0x7fffffff,
} ufbx_unicode_error_handling;

// -- Main API

// Options for `ufbx_load_file/memory/stream/stdio()`
// NOTE: Initialize to zero with `{ 0 }` (C) or `{ }` (C++)
typedef struct ufbx_load_opts {
	uint32_t _begin_zero; 

	ufbx_allocator_opts temp_allocator;   // < Allocator used during loading
	ufbx_allocator_opts result_allocator; // < Allocator used for the final scene

	// Preferences
	bool ignore_geometry;   // < Do not load geometry datsa (vertices, indices, etc)
	bool ignore_animation;  // < Do not load animation curves
	bool ignore_embedded;   // < Do not load embedded content
	bool evaluate_skinning; // < Evaluate skinning (see ufbx_mesh.skinned_vertices)
	bool evaluate_caches;   // < Evaluate vertex caches (see ufbx_mesh.skinned_vertices)

	// WARNING: Potentially unsafe! Try to open external files such as geometry caches
	bool load_external_files;

	// Don't compute `ufbx_skin_deformer` `vertices` and `weights` arrays saving
	// a bit of memory and time if not needed
	bool skip_skin_vertices;

	// Don't adjust reading the FBX file depending on the detected exporter
	bool disable_quirks;

	// Don't allow partially broken FBX files to load
	bool strict;

	// UNSAFE: If enabled allows using unsafe options that may fundamentally
	// break the API guarantees.
	ufbx_unsafe bool allow_unsafe;

	// Specify how to handle broken indices.
	ufbx_index_error_handling index_error_handling;

	// Connect related elements even if they are broken. If `false` (default)
	// `ufbx_skin_cluster` with a missing `bone` field are _not_ included in
	// the `ufbx_skin_deformer.clusters[]` array for example.
	bool connect_broken_elements;

	// Allow nodes that are not connected in any way to the root. Conversely if
	// disabled, all lone nodes will be parented under `ufbx_scene.root_node`.
	bool allow_nodes_out_of_root;

	// If a mesh does not have a material create a `ufbx_mesh_material` part
	// with a `NULL` material pointer. This can be more convenient if you need
	// to split models into parts per material.
	bool allow_null_material;

	// Generate vertex normals for a meshes that are missing normals.
	bool generate_missing_normals;

	// Path separator character, defaults to '\' on Windows and '/' otherwise.
	char path_separator;

	// Estimated file size for progress reporting
	uint64_t file_size_estimate;

	// Buffer size in bytes to use for reading from files or IO callbacks
	size_t read_buffer_size;

	// Filename to use as a base for relative file paths if not specified using
	// `ufbx_load_file()`. Use `length = SIZE_MAX` for NULL-terminated strings.
	// `raw_filename` will be derived from this if empty.
	ufbx_string filename;

	// Raw non-UTF8 filename. Does not support NULL termination.
	// `filename` will be derived from this if empty.
	ufbx_blob raw_filename;

	// Progress reporting
	ufbx_progress_cb progress_cb;
	uint64_t progress_interval_hint; // < Bytes between progress report calls

	// External file callbacks (defaults to stdio.h)
	ufbx_open_file_cb open_file_cb;

	// Apply an implicit root transformation to match axes.
	// Used if `ufbx_coordinate_axes_valid(target_axes)`.
	ufbx_coordinate_axes target_axes;

	// Scale the scene so that one world-space unit is `target_unit_meters` meters.
	// By default units are not scaled.
	ufbx_real target_unit_meters;

	// Do not scale necessary properties curves with `target_unit_meters`.
	bool no_prop_unit_scaling;

	// Do not scale necessary animation curves with `target_unit_meters`.
	bool no_anim_curve_unit_scaling;

	// Override for the root transform
	bool use_root_transform;
	ufbx_transform root_transform;

	// Specify how to handle Unicode errors in strings.
	ufbx_unicode_error_handling unicode_error_handling;

	// Retain the raw document structure using `ufbx_dom_node`.
	bool retain_dom;

	uint32_t _end_zero; 
} ufbx_load_opts;

// Options for `ufbx_evaluate_scene()`
// NOTE: Initialize to zero with `{ 0 }` (C) or `{ }` (C++)
typedef struct ufbx_evaluate_opts {
	uint32_t _begin_zero;

	ufbx_allocator_opts temp_allocator;   // < Allocator used during evaluation
	ufbx_allocator_opts result_allocator; // < Allocator used for the final scene

	bool evaluate_skinning; // < Evaluate skinning (see ufbx_mesh.skinned_vertices)
	bool evaluate_caches;   // < Evaluate vertex caches (see ufbx_mesh.skinned_vertices)

	// WARNING: Potentially unsafe! Try to open external files such as geometry caches
	bool load_external_files;

	// External file callbacks (defaults to stdio.h)
	ufbx_open_file_cb open_file_cb;

	uint32_t _end_zero;
} ufbx_evaluate_opts;

// Options for `ufbx_tessellate_nurbs_curve()`
// NOTE: Initialize to zero with `{ 0 }` (C) or `{ }` (C++)
typedef struct ufbx_tessellate_curve_opts {
	uint32_t _begin_zero;

	ufbx_allocator_opts temp_allocator;   // < Allocator used during tessellation
	ufbx_allocator_opts result_allocator; // < Allocator used for the final line curve

	// How many segments tessellate each step in `ufbx_nurbs_basis.steps`.
	uint32_t span_subdivision;

	uint32_t _end_zero;
} ufbx_tessellate_curve_opts;

// Options for `ufbx_tessellate_nurbs_surface()`
// NOTE: Initialize to zero with `{ 0 }` (C) or `{ }` (C++)
typedef struct ufbx_tessellate_surface_opts {
	uint32_t _begin_zero;

	ufbx_allocator_opts temp_allocator;   // < Allocator used during tessellation
	ufbx_allocator_opts result_allocator; // < Allocator used for the final mesh

	// How many segments tessellate each step in `ufbx_nurbs_basis.steps`.
	// NOTE: Default is `4`, _not_ `ufbx_nurbs_surface.span_subdivision_u/v` as that
	// would make it easy to create an FBX file with an absurdly high subdivision
	// rate (similar to mesh subdivision). Please enforce copy the value yourself
	// enforcing whatever limits you deem reasonable.
	uint32_t span_subdivision_u;
	uint32_t span_subdivision_v;

	uint32_t _end_zero;
} ufbx_tessellate_surface_opts;

// Options for `ufbx_subdivide_mesh()`
// NOTE: Initialize to zero with `{ 0 }` (C) or `{ }` (C++)
typedef struct ufbx_subdivide_opts {
	uint32_t _begin_zero;

	ufbx_allocator_opts temp_allocator;   // < Allocator used during subdivision
	ufbx_allocator_opts result_allocator; // < Allocator used for the final mesh

	ufbx_subdivision_boundary boundary;
	ufbx_subdivision_boundary uv_boundary;

	// Do not generate normals
	bool ignore_normals;

	// Interpolate existing normals using the subdivision rules
	// instead of generating new normals
	bool interpolate_normals;

	// Subdivide also tangent attributes
	bool interpolate_tangents;

	// Map subdivided vertices into weighted original vertices.
	// NOTE: May be O(n^2) if `max_vertex_sources` is not specified!
	bool evaluate_source_vertices;

	// Limit source vertices per subdivided vertex.
	size_t max_source_vertices;

	// Calculate bone influences over subdivided vertices (if applicable).
	bool evaluate_skin_weights;

	// Limit bone influences per subdivided vertex.
	size_t max_skin_weights;

	// Index of the skin deformer to use for `evaluate_skin_weights`.
	size_t skin_deformer_index;

	uint32_t _end_zero;
} ufbx_subdivide_opts;

// Options for `ufbx_load_geometry_cache()`
// NOTE: Initialize to zero with `{ 0 }` (C) or `{ }` (C++)
typedef struct ufbx_geometry_cache_opts {
	uint32_t _begin_zero;

	ufbx_allocator_opts temp_allocator;   // < Allocator used during loading
	ufbx_allocator_opts result_allocator; // < Allocator used for the final scene

	// External file callbacks (defaults to stdio.h)
	ufbx_open_file_cb open_file_cb;

	// FPS value for converting frame times to seconds
	double frames_per_second;

	uint32_t _end_zero;
} ufbx_geometry_cache_opts;

// Options for `ufbx_read_geometry_cache_*()`
// NOTE: Initialize to zero with `{ 0 }` (C) or `{ }` (C++)
typedef struct ufbx_geometry_cache_data_opts {
	uint32_t _begin_zero;

	// External file callbacks (defaults to stdio.h)
	ufbx_open_file_cb open_file_cb;

	bool additive;
	bool use_weight;
	ufbx_real weight;

	uint32_t _end_zero;
} ufbx_geometry_cache_data_opts;

typedef struct ufbx_panic {
	bool did_panic;
	size_t message_length;
	char message[UFBX_PANIC_MESSAGE_LENGTH];
} ufbx_panic;

// -- API

#ifdef __cplusplus
extern "C" {
#endif

// Various zero/empty/identity values
extern const ufbx_string ufbx_empty_string;
extern const ufbx_blob ufbx_empty_blob;
extern const ufbx_matrix ufbx_identity_matrix;
extern const ufbx_transform ufbx_identity_transform;
extern const ufbx_vec2 ufbx_zero_vec2;
extern const ufbx_vec3 ufbx_zero_vec3;
extern const ufbx_vec4 ufbx_zero_vec4;
extern const ufbx_quat ufbx_identity_quat;

// Commonly used coordinate axes

extern const ufbx_coordinate_axes ufbx_axes_right_handed_y_up;
extern const ufbx_coordinate_axes ufbx_axes_right_handed_z_up;
extern const ufbx_coordinate_axes ufbx_axes_left_handed_y_up;
extern const ufbx_coordinate_axes ufbx_axes_left_handed_z_up;

// Sizes of element types. eg `sizeof(ufbx_node)`
extern const size_t ufbx_element_type_size[UFBX_ELEMENT_TYPE_COUNT];

// Version of the source file, comparable to `UFBX_HEADER_VERSION`
extern const uint32_t ufbx_source_version;

// Practically always `true` (see below), if not you need to be careful with threads.
//
// Guaranteed to be `true` in _any_ of the following conditions:
// - ufbx.c has been compiled using: GCC / Clang / MSVC / ICC / EMCC / TCC
// - ufbx.c has been compiled as C++11 or later
// - ufbx.c has been compiled as C11 or later with `<stdatomic.h>` support
//
// If `false` you can't call the following functions concurrently:
//   ufbx_evaluate_scene()
//   ufbx_free_scene()
//   ufbx_subdivide_mesh()
//   ufbx_tessellate_nurbs_surface()
//   ufbx_free_mesh()
ufbx_abi bool ufbx_is_thread_safe(void);

// Load a scene from a `size` byte memory buffer at `data`
ufbx_abi ufbx_scene *ufbx_load_memory(
	const void *data, size_t data_size,
	const ufbx_load_opts *opts, ufbx_error *error);

// Load a scene by opening a file named `filename`
ufbx_abi ufbx_scene *ufbx_load_file(
	const char *filename,
	const ufbx_load_opts *opts, ufbx_error *error);
ufbx_abi ufbx_scene *ufbx_load_file_len(
	const char *filename, size_t filename_len,
	const ufbx_load_opts *opts, ufbx_error *error);

// Load a scene by reading from an `FILE *file` stream
// NOTE: `file` is passed as a `void` pointer to avoid including <stdio.h>
ufbx_abi ufbx_scene *ufbx_load_stdio(
	void *file,
	const ufbx_load_opts *opts, ufbx_error *error);

// Load a scene by reading from an `FILE *file` stream with a prefix
// NOTE: `file` is passed as a `void` pointer to avoid including <stdio.h>
ufbx_abi ufbx_scene *ufbx_load_stdio_prefix(
	void *file,
	const void *prefix, size_t prefix_size,
	const ufbx_load_opts *opts, ufbx_error *error);

// Load a scene from a user-specified stream
ufbx_abi ufbx_scene *ufbx_load_stream(
	const ufbx_stream *stream,
	const ufbx_load_opts *opts, ufbx_error *error);

// Load a scene from a user-specified stream with a prefix
ufbx_abi ufbx_scene *ufbx_load_stream_prefix(
	const ufbx_stream *stream,
	const void *prefix, size_t prefix_size,
	const ufbx_load_opts *opts, ufbx_error *error);

// Free a previously loaded or evaluated scene
ufbx_abi void ufbx_free_scene(ufbx_scene *scene);

// Increment `scene` refcount
ufbx_abi void ufbx_retain_scene(ufbx_scene *scene);

// Format a textual description of `error`.
// Always produces a NULL-terminated string to `char dst[dst_size]`, truncating if
// necessary. Returns the number of characters written not including the NULL terminator.
ufbx_abi size_t ufbx_format_error(char *dst, size_t dst_size, const ufbx_error *error);

// Query

// Find a property `name` from `props`, returns `NULL` if not found.
// Searches through `ufbx_props.defaults` as well.
ufbx_abi ufbx_prop *ufbx_find_prop_len(const ufbx_props *props, const char *name, size_t name_len);
ufbx_inline ufbx_prop *ufbx_find_prop(const ufbx_props *props, const char *name) { return ufbx_find_prop_len(props, name, strlen(name));}

// Utility functions for finding the value of a property, returns `def` if not found.
// NOTE: For `ufbx_string` you need to ensure the lifetime of the default is
// sufficient as no copy is made.
ufbx_abi ufbx_real ufbx_find_real_len(const ufbx_props *props, const char *name, size_t name_len, ufbx_real def);
ufbx_inline ufbx_real ufbx_find_real(const ufbx_props *props, const char *name, ufbx_real def) { return ufbx_find_real_len(props, name, strlen(name), def); }
ufbx_abi ufbx_vec3 ufbx_find_vec3_len(const ufbx_props *props, const char *name, size_t name_len, ufbx_vec3 def);
ufbx_inline ufbx_vec3 ufbx_find_vec3(const ufbx_props *props, const char *name, ufbx_vec3 def) { return ufbx_find_vec3_len(props, name, strlen(name), def); }
ufbx_abi int64_t ufbx_find_int_len(const ufbx_props *props, const char *name, size_t name_len, int64_t def);
ufbx_inline int64_t ufbx_find_int(const ufbx_props *props, const char *name, int64_t def) { return ufbx_find_int_len(props, name, strlen(name), def); }
ufbx_abi bool ufbx_find_bool_len(const ufbx_props *props, const char *name, size_t name_len, bool def);
ufbx_inline bool ufbx_find_bool(const ufbx_props *props, const char *name, bool def) { return ufbx_find_bool_len(props, name, strlen(name), def); }
ufbx_abi ufbx_string ufbx_find_string_len(const ufbx_props *props, const char *name, size_t name_len, ufbx_string def);
ufbx_inline ufbx_string ufbx_find_string(const ufbx_props *props, const char *name, ufbx_string def) { return ufbx_find_string_len(props, name, strlen(name), def); }
ufbx_abi ufbx_blob ufbx_find_blob_len(const ufbx_props *props, const char *name, size_t name_len, ufbx_blob def);
ufbx_inline ufbx_blob ufbx_find_blob(const ufbx_props *props, const char *name, ufbx_blob def) { return ufbx_find_blob_len(props, name, strlen(name), def); }

// Get an element connected to a property.
ufbx_abi ufbx_element *ufbx_get_prop_element(const ufbx_element *element, const ufbx_prop *prop, ufbx_element_type type);

// Find any element of type `type` in `scene` by `name`.
// For example if you want to find `ufbx_material` named `Mat`:
//   (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Mat");
ufbx_abi ufbx_element *ufbx_find_element_len(const ufbx_scene *scene, ufbx_element_type type, const char *name, size_t name_len);
ufbx_inline ufbx_element *ufbx_find_element(const ufbx_scene *scene, ufbx_element_type type, const char *name) { return ufbx_find_element_len(scene, type, name, strlen(name)); }

// Find node in `scene` by `name` (shorthand for `ufbx_find_element(UFBX_ELEMENT_NODE)`).
ufbx_abi ufbx_node *ufbx_find_node_len(const ufbx_scene *scene, const char *name, size_t name_len);
ufbx_inline ufbx_node *ufbx_find_node(const ufbx_scene *scene, const char *name) { return ufbx_find_node_len(scene, name, strlen(name)); }

// Find an animation stack in `scene` by `name` (shorthand for `ufbx_find_element(UFBX_ELEMENT_ANIM_STACK)`)
ufbx_abi ufbx_anim_stack *ufbx_find_anim_stack_len(const ufbx_scene *scene, const char *name, size_t name_len);
ufbx_inline ufbx_anim_stack *ufbx_find_anim_stack(const ufbx_scene *scene, const char *name) { return ufbx_find_anim_stack_len(scene, name, strlen(name)); }

// Find a single animated property `prop` of `element` in `layer`.
// Returns `NULL` if not found.
ufbx_abi ufbx_anim_prop *ufbx_find_anim_prop_len(const ufbx_anim_layer *layer, const ufbx_element *element, const char *prop, size_t prop_len);
ufbx_inline ufbx_anim_prop *ufbx_find_anim_prop(const ufbx_anim_layer *layer, const ufbx_element *element, const char *prop) { return ufbx_find_anim_prop_len(layer, element, prop, strlen(prop)); }

// Find all animated properties of `element` in `layer`.
ufbx_abi ufbx_anim_prop_list ufbx_find_anim_props(const ufbx_anim_layer *layer, const ufbx_element *element);

// Get a matrix that transforms normals in the same way as Autodesk software.
// NOTE: The resulting normals are slightly incorrect as this function deliberately
// inverts geometric transformation wrong. For better results use
// `ufbx_matrix_for_normals(&node->geometry_to_world)`.
ufbx_abi ufbx_matrix ufbx_get_compatible_matrix_for_normals(const ufbx_node *node);

// Utility

// Decompress a DEFLATE compressed buffer.
// Returns the decompressed size or a negative error code (see source for details).
// NOTE: You must supply a valid `retain` with `ufbx_inflate_retain.initialized == false`
// but the rest can be uninitialized.
ufbx_abi ptrdiff_t ufbx_inflate(void *dst, size_t dst_size, const ufbx_inflate_input *input, ufbx_inflate_retain *retain);

// Open a `ufbx_stream` from a file.
// Use `path_len == SIZE_MAX` for NULL terminated string.
// NOTE: `user` is not used, it exists only for compatability to `ufbx_open_file_fn`
ufbx_abi bool ufbx_open_file(void *user, ufbx_stream *stream, const char *path, size_t path_len);

// Animation evaluation

// Evaluate a single animation `curve` at a `time`.
// Returns `default_value` only if `curve == NULL` or it has no keyframes.
ufbx_abi ufbx_real ufbx_evaluate_curve(const ufbx_anim_curve *curve, double time, ufbx_real default_value);

// Evaluate a value from bundled animation curves.
ufbx_abi ufbx_real ufbx_evaluate_anim_value_real(const ufbx_anim_value *anim_value, double time);
ufbx_abi ufbx_vec2 ufbx_evaluate_anim_value_vec2(const ufbx_anim_value *anim_value, double time);
ufbx_abi ufbx_vec3 ufbx_evaluate_anim_value_vec3(const ufbx_anim_value *anim_value, double time);

// Evaluate an animated property `name` from `element` at `time`.
// NOTE: If the property is not found it will have the flag `UFBX_PROP_FLAG_NOT_FOUND`.
ufbx_abi ufbx_prop ufbx_evaluate_prop_len(const ufbx_anim *anim, const ufbx_element *element, const char *name, size_t name_len, double time);
ufbx_inline ufbx_prop ufbx_evaluate_prop(const ufbx_anim *anim, const ufbx_element *element, const char *name, double time) {
	return ufbx_evaluate_prop_len(anim, element, name, strlen(name), time);
}

// Evaluate all _animated_ properties of `element`.
// HINT: This function returns an `ufbx_props` structure with the original properties as
// `ufbx_props.defaults`. This lets you use `ufbx_find_prop/value()` for the results.
ufbx_abi ufbx_props ufbx_evaluate_props(const ufbx_anim *anim, const ufbx_element *element, double time, ufbx_prop *buffer, size_t buffer_size);

ufbx_abi ufbx_transform ufbx_evaluate_transform(const ufbx_anim *anim, const ufbx_node *node, double time);
ufbx_abi ufbx_real ufbx_evaluate_blend_weight(const ufbx_anim *anim, const ufbx_blend_channel *channel, double time);

ufbx_abi ufbx_const_prop_override_list ufbx_prepare_prop_overrides(ufbx_prop_override *overrides, size_t num_overrides);

// Evaluate the whole `scene` at a specific `time` in the animation `anim`.
// The returned scene behaves as if it had been exported at a specific time
// in the specified animation, except that animated elements' properties contain
// only the animated values, the original ones are in `props->defaults`.
//
// NOTE: The returned scene refers to the original `scene` so the original
// scene cannot be freed until all evaluated scenes are freed.
ufbx_abi ufbx_scene *ufbx_evaluate_scene(const ufbx_scene *scene, const ufbx_anim *anim, double time, const ufbx_evaluate_opts *opts, ufbx_error *error);

// Materials

ufbx_abi ufbx_texture *ufbx_find_prop_texture_len(const ufbx_material *material, const char *name, size_t name_len);
ufbx_inline ufbx_texture *ufbx_find_prop_texture(const ufbx_material *material, const char *name) {
	return ufbx_find_prop_texture_len(material, name, strlen(name));
}

ufbx_abi ufbx_string ufbx_find_shader_prop_len(const ufbx_shader *shader, const char *name, size_t name_len);
ufbx_inline ufbx_string ufbx_find_shader_prop(const ufbx_shader *shader, const char *name) {
	return ufbx_find_shader_prop_len(shader, name, strlen(name));
}

ufbx_abi ufbx_shader_prop_binding_list ufbx_find_shader_prop_bindings_len(const ufbx_shader *shader, const char *name, size_t name_len);
ufbx_inline ufbx_shader_prop_binding_list ufbx_find_shader_prop_bindings(const ufbx_shader *shader, const char *name) {
	return ufbx_find_shader_prop_bindings_len(shader, name, strlen(name));
}

ufbx_abi ufbx_shader_texture_input *ufbx_find_shader_texture_input_len(const ufbx_shader_texture *shader, const char *name, size_t name_len);
ufbx_inline ufbx_shader_texture_input *ufbx_find_shader_texture_input(const ufbx_shader_texture *shader, const char *name) {
	return ufbx_find_shader_texture_input_len(shader, name, strlen(name));
}

// Math

ufbx_abi bool ufbx_coordinate_axes_valid(ufbx_coordinate_axes axes);

ufbx_abi ufbx_real ufbx_quat_dot(ufbx_quat a, ufbx_quat b);
ufbx_abi ufbx_quat ufbx_quat_mul(ufbx_quat a, ufbx_quat b);
ufbx_abi ufbx_quat ufbx_quat_normalize(ufbx_quat q);
ufbx_abi ufbx_quat ufbx_quat_fix_antipodal(ufbx_quat q, ufbx_quat reference);
ufbx_abi ufbx_quat ufbx_quat_slerp(ufbx_quat a, ufbx_quat b, ufbx_real t);
ufbx_abi ufbx_vec3 ufbx_quat_rotate_vec3(ufbx_quat q, ufbx_vec3 v);
ufbx_abi ufbx_vec3 ufbx_quat_to_euler(ufbx_quat q, ufbx_rotation_order order);
ufbx_abi ufbx_quat ufbx_euler_to_quat(ufbx_vec3 v, ufbx_rotation_order order);

ufbx_abi ufbx_matrix ufbx_matrix_mul(const ufbx_matrix *a, const ufbx_matrix *b);
ufbx_abi ufbx_real ufbx_matrix_determinant(const ufbx_matrix *m);
ufbx_abi ufbx_matrix ufbx_matrix_invert(const ufbx_matrix *m);
ufbx_abi ufbx_matrix ufbx_matrix_for_normals(const ufbx_matrix *m);
ufbx_abi ufbx_vec3 ufbx_transform_position(const ufbx_matrix *m, ufbx_vec3 v);
ufbx_abi ufbx_vec3 ufbx_transform_direction(const ufbx_matrix *m, ufbx_vec3 v);
ufbx_abi ufbx_matrix ufbx_transform_to_matrix(const ufbx_transform *t);
ufbx_abi ufbx_transform ufbx_matrix_to_transform(const ufbx_matrix *m);

// Skinning

ufbx_abi ufbx_matrix ufbx_catch_get_skin_vertex_matrix(ufbx_panic *panic, const ufbx_skin_deformer *skin, size_t vertex, const ufbx_matrix *fallback);
ufbx_inline ufbx_matrix ufbx_get_skin_vertex_matrix(const ufbx_skin_deformer *skin, size_t vertex, const ufbx_matrix *fallback) {
	return ufbx_catch_get_skin_vertex_matrix(NULL, skin, vertex, fallback);
}

ufbx_abi ufbx_vec3 ufbx_get_blend_shape_vertex_offset(const ufbx_blend_shape *shape, size_t vertex);
ufbx_abi ufbx_vec3 ufbx_get_blend_vertex_offset(const ufbx_blend_deformer *blend, size_t vertex);

ufbx_abi void ufbx_add_blend_shape_vertex_offsets(const ufbx_blend_shape *shape, ufbx_vec3 *vertices, size_t num_vertices, ufbx_real weight);
ufbx_abi void ufbx_add_blend_vertex_offsets(const ufbx_blend_deformer *blend, ufbx_vec3 *vertices, size_t num_vertices, ufbx_real weight);

// Curves/surfaces

ufbx_abi size_t ufbx_evaluate_nurbs_basis(const ufbx_nurbs_basis *basis, ufbx_real u, ufbx_real *weights, size_t num_weights, ufbx_real *derivatives, size_t num_derivatives);

ufbx_abi ufbx_curve_point ufbx_evaluate_nurbs_curve(const ufbx_nurbs_curve *curve, ufbx_real u);
ufbx_abi ufbx_surface_point ufbx_evaluate_nurbs_surface(const ufbx_nurbs_surface *surface, ufbx_real u, ufbx_real v);

ufbx_abi ufbx_line_curve *ufbx_tessellate_nurbs_curve(const ufbx_nurbs_curve *curve, const ufbx_tessellate_curve_opts *opts, ufbx_error *error);
ufbx_abi ufbx_mesh *ufbx_tessellate_nurbs_surface(const ufbx_nurbs_surface *surface, const ufbx_tessellate_surface_opts *opts, ufbx_error *error);

ufbx_abi void ufbx_free_line_curve(ufbx_line_curve *curve);
ufbx_abi void ufbx_retain_line_curve(ufbx_line_curve *curve);

// Mesh Topology

ufbx_abi uint32_t ufbx_catch_triangulate_face(ufbx_panic *panic, uint32_t *indices, size_t num_indices, const ufbx_mesh *mesh, ufbx_face face);
ufbx_inline uint32_t ufbx_triangulate_face(uint32_t *indices, size_t num_indices, const ufbx_mesh *mesh, ufbx_face face) {
	return ufbx_catch_triangulate_face(NULL, indices, num_indices, mesh, face);
}

// Generate the half-edge representation of `mesh` to `topo[mesh->num_indices]`
ufbx_abi void ufbx_catch_compute_topology(ufbx_panic *panic, const ufbx_mesh *mesh, ufbx_topo_edge *topo, size_t num_topo);
ufbx_inline void ufbx_compute_topology(const ufbx_mesh *mesh, ufbx_topo_edge *topo, size_t num_topo) {
	ufbx_catch_compute_topology(NULL, mesh, topo, num_topo);
}

// Get the next/previous edge around a vertex
// NOTE: Does not return the half-edge on the opposite side (ie. `topo[index].twin`)

ufbx_abi uint32_t ufbx_catch_topo_next_vertex_edge(ufbx_panic *panic, const ufbx_topo_edge *topo, size_t num_topo, uint32_t index);
ufbx_inline uint32_t ufbx_topo_next_vertex_edge(const ufbx_topo_edge *topo, size_t num_topo, uint32_t index) {
	return ufbx_catch_topo_next_vertex_edge(NULL, topo, num_topo, index);
}

ufbx_abi uint32_t ufbx_catch_topo_prev_vertex_edge(ufbx_panic *panic, const ufbx_topo_edge *topo, size_t num_topo, uint32_t index);
ufbx_inline uint32_t ufbx_topo_prev_vertex_edge(const ufbx_topo_edge *topo, size_t num_topo, uint32_t index) {
	return ufbx_catch_topo_prev_vertex_edge(NULL, topo, num_topo, index);
}

ufbx_abi ufbx_vec3 ufbx_catch_get_weighted_face_normal(ufbx_panic *panic, const ufbx_vertex_vec3 *positions, ufbx_face face);
ufbx_inline ufbx_vec3 ufbx_get_weighted_face_normal(const ufbx_vertex_vec3 *positions, ufbx_face face) {
	return ufbx_catch_get_weighted_face_normal(NULL, positions, face);
}

ufbx_abi size_t ufbx_catch_generate_normal_mapping(ufbx_panic *panic, const ufbx_mesh *mesh,
	const ufbx_topo_edge *topo, size_t num_topo,
	uint32_t *normal_indices, size_t num_normal_indices, bool assume_smooth);
ufbx_abi size_t ufbx_generate_normal_mapping(const ufbx_mesh *mesh,
	const ufbx_topo_edge *topo, size_t num_topo,
	uint32_t *normal_indices, size_t num_normal_indices, bool assume_smooth);

ufbx_abi void ufbx_catch_compute_normals(ufbx_panic *panic, const ufbx_mesh *mesh, const ufbx_vertex_vec3 *positions,
	const uint32_t *normal_indices, size_t num_normal_indices,
	ufbx_vec3 *normals, size_t num_normals);
ufbx_abi void ufbx_compute_normals(const ufbx_mesh *mesh, const ufbx_vertex_vec3 *positions,
	const uint32_t *normal_indices, size_t num_normal_indices,
	ufbx_vec3 *normals, size_t num_normals);

ufbx_abi ufbx_mesh *ufbx_subdivide_mesh(const ufbx_mesh *mesh, size_t level, const ufbx_subdivide_opts *opts, ufbx_error *error);

ufbx_abi void ufbx_free_mesh(ufbx_mesh *mesh);
ufbx_abi void ufbx_retain_mesh(ufbx_mesh *mesh);

// Geometry caches

ufbx_abi ufbx_geometry_cache *ufbx_load_geometry_cache(
	const char *filename,
	const ufbx_geometry_cache_opts *opts, ufbx_error *error);
ufbx_abi ufbx_geometry_cache *ufbx_load_geometry_cache_len(
	const char *filename, size_t filename_len,
	const ufbx_geometry_cache_opts *opts, ufbx_error *error);

ufbx_abi void ufbx_free_geometry_cache(ufbx_geometry_cache *cache);
ufbx_abi void ufbx_retain_geometry_cache(ufbx_geometry_cache *cache);

ufbx_abi size_t ufbx_get_read_geometry_cache_real_num_data(const ufbx_cache_frame *frame);
ufbx_abi size_t ufbx_get_sample_geometry_cache_real_num_data(const ufbx_cache_channel *channel, double time);
ufbx_abi size_t ufbx_get_read_geometry_cache_vec3_num_data(const ufbx_cache_frame *frame);
ufbx_abi size_t ufbx_get_sample_geometry_cache_vec3_num_data(const ufbx_cache_channel *channel, double time);

ufbx_abi size_t ufbx_read_geometry_cache_real(const ufbx_cache_frame *frame, ufbx_real *data, size_t num_data, const ufbx_geometry_cache_data_opts *opts);
ufbx_abi size_t ufbx_sample_geometry_cache_real(const ufbx_cache_channel *channel, double time, ufbx_real *data, size_t num_data, const ufbx_geometry_cache_data_opts *opts);
ufbx_abi size_t ufbx_read_geometry_cache_vec3(const ufbx_cache_frame *frame, ufbx_vec3 *data, size_t num_data, const ufbx_geometry_cache_data_opts *opts);
ufbx_abi size_t ufbx_sample_geometry_cache_vec3(const ufbx_cache_channel *channel, double time, ufbx_vec3 *data, size_t num_data, const ufbx_geometry_cache_data_opts *opts);

// DOM

ufbx_abi ufbx_dom_node *ufbx_dom_find_len(const ufbx_dom_node *parent, const char *name, size_t name_len);
ufbx_inline ufbx_dom_node *ufbx_dom_find(const ufbx_dom_node *parent, const char *name) { return ufbx_dom_find_len(parent, name, strlen(name)); }

// Utility

ufbx_abi size_t ufbx_generate_indices(const ufbx_vertex_stream *streams, size_t num_streams, uint32_t *indices, size_t num_indices, const ufbx_allocator_opts *allocator, ufbx_error *error);

// -- Inline API

ufbx_abi ufbx_real ufbx_catch_get_vertex_real(ufbx_panic *panic, const ufbx_vertex_real *v, size_t index);
ufbx_abi ufbx_vec2 ufbx_catch_get_vertex_vec2(ufbx_panic *panic, const ufbx_vertex_vec2 *v, size_t index);
ufbx_abi ufbx_vec3 ufbx_catch_get_vertex_vec3(ufbx_panic *panic, const ufbx_vertex_vec3 *v, size_t index);
ufbx_abi ufbx_vec4 ufbx_catch_get_vertex_vec4(ufbx_panic *panic, const ufbx_vertex_vec4 *v, size_t index);

ufbx_inline ufbx_real ufbx_get_vertex_real(const ufbx_vertex_real *v, size_t index) { ufbx_assert(index < v->indices.count); return v->values.data[(int32_t)v->indices.data[index]]; }
ufbx_inline ufbx_vec2 ufbx_get_vertex_vec2(const ufbx_vertex_vec2 *v, size_t index) { ufbx_assert(index < v->indices.count); return v->values.data[(int32_t)v->indices.data[index]]; }
ufbx_inline ufbx_vec3 ufbx_get_vertex_vec3(const ufbx_vertex_vec3 *v, size_t index) { ufbx_assert(index < v->indices.count); return v->values.data[(int32_t)v->indices.data[index]]; }
ufbx_inline ufbx_vec4 ufbx_get_vertex_vec4(const ufbx_vertex_vec4 *v, size_t index) { ufbx_assert(index < v->indices.count); return v->values.data[(int32_t)v->indices.data[index]]; }

ufbx_abi size_t ufbx_get_triangulate_face_num_indices(ufbx_face face);

ufbx_abi ufbx_unknown *ufbx_as_unknown(const ufbx_element *element);
ufbx_abi ufbx_node *ufbx_as_node(const ufbx_element *element);
ufbx_abi ufbx_mesh *ufbx_as_mesh(const ufbx_element *element);
ufbx_abi ufbx_light *ufbx_as_light(const ufbx_element *element);
ufbx_abi ufbx_camera *ufbx_as_camera(const ufbx_element *element);
ufbx_abi ufbx_bone *ufbx_as_bone(const ufbx_element *element);
ufbx_abi ufbx_empty *ufbx_as_empty(const ufbx_element *element);
ufbx_abi ufbx_line_curve *ufbx_as_line_curve(const ufbx_element *element);
ufbx_abi ufbx_nurbs_curve *ufbx_as_nurbs_curve(const ufbx_element *element);
ufbx_abi ufbx_nurbs_surface *ufbx_as_nurbs_surface(const ufbx_element *element);
ufbx_abi ufbx_nurbs_trim_surface *ufbx_as_nurbs_trim_surface(const ufbx_element *element);
ufbx_abi ufbx_nurbs_trim_boundary *ufbx_as_nurbs_trim_boundary(const ufbx_element *element);
ufbx_abi ufbx_procedural_geometry *ufbx_as_procedural_geometry(const ufbx_element *element);
ufbx_abi ufbx_stereo_camera *ufbx_as_stereo_camera(const ufbx_element *element);
ufbx_abi ufbx_camera_switcher *ufbx_as_camera_switcher(const ufbx_element *element);
ufbx_abi ufbx_marker *ufbx_as_marker(const ufbx_element *element);
ufbx_abi ufbx_lod_group *ufbx_as_lod_group(const ufbx_element *element);
ufbx_abi ufbx_skin_deformer *ufbx_as_skin_deformer(const ufbx_element *element);
ufbx_abi ufbx_skin_cluster *ufbx_as_skin_cluster(const ufbx_element *element);
ufbx_abi ufbx_blend_deformer *ufbx_as_blend_deformer(const ufbx_element *element);
ufbx_abi ufbx_blend_channel *ufbx_as_blend_channel(const ufbx_element *element);
ufbx_abi ufbx_blend_shape *ufbx_as_blend_shape(const ufbx_element *element);
ufbx_abi ufbx_cache_deformer *ufbx_as_cache_deformer(const ufbx_element *element);
ufbx_abi ufbx_cache_file *ufbx_as_cache_file(const ufbx_element *element);
ufbx_abi ufbx_material *ufbx_as_material(const ufbx_element *element);
ufbx_abi ufbx_texture *ufbx_as_texture(const ufbx_element *element);
ufbx_abi ufbx_video *ufbx_as_video(const ufbx_element *element);
ufbx_abi ufbx_shader *ufbx_as_shader(const ufbx_element *element);
ufbx_abi ufbx_shader_binding *ufbx_as_shader_binding(const ufbx_element *element);
ufbx_abi ufbx_anim_stack *ufbx_as_anim_stack(const ufbx_element *element);
ufbx_abi ufbx_anim_layer *ufbx_as_anim_layer(const ufbx_element *element);
ufbx_abi ufbx_anim_value *ufbx_as_anim_value(const ufbx_element *element);
ufbx_abi ufbx_anim_curve *ufbx_as_anim_curve(const ufbx_element *element);
ufbx_abi ufbx_display_layer *ufbx_as_display_layer(const ufbx_element *element);
ufbx_abi ufbx_selection_set *ufbx_as_selection_set(const ufbx_element *element);
ufbx_abi ufbx_selection_node *ufbx_as_selection_node(const ufbx_element *element);
ufbx_abi ufbx_character *ufbx_as_character(const ufbx_element *element);
ufbx_abi ufbx_constraint *ufbx_as_constraint(const ufbx_element *element);
ufbx_abi ufbx_pose *ufbx_as_pose(const ufbx_element *element);
ufbx_abi ufbx_metadata_object *ufbx_as_metadata_object(const ufbx_element *element);

// -- FFI API

ufbx_abi void ufbx_ffi_find_int_len(int64_t *retval, const ufbx_props *props, const char *name, size_t name_len, const int64_t *def);
ufbx_abi void ufbx_ffi_find_vec3_len(ufbx_vec3 *retval, const ufbx_props *props, const char *name, size_t name_len, const ufbx_vec3 *def);
ufbx_abi void ufbx_ffi_find_string_len(ufbx_string *retval, const ufbx_props *props, const char *name, size_t name_len, const ufbx_string *def);
ufbx_abi void ufbx_ffi_find_anim_props(ufbx_anim_prop_list *retval, const ufbx_anim_layer *layer, const ufbx_element *element);
ufbx_abi void ufbx_ffi_get_compatible_matrix_for_normals(ufbx_matrix *retval, const ufbx_node *node);
ufbx_abi void ufbx_ffi_evaluate_anim_value_vec2(ufbx_vec2 *retval, const ufbx_anim_value *anim_value, double time);
ufbx_abi void ufbx_ffi_evaluate_anim_value_vec3(ufbx_vec3 *retval, const ufbx_anim_value *anim_value, double time);
ufbx_abi void ufbx_ffi_evaluate_prop_len(ufbx_prop *retval, const ufbx_anim *anim, const ufbx_element *element, const char *name, size_t name_len, double time);
ufbx_abi void ufbx_ffi_evaluate_props(ufbx_props *retval, const ufbx_anim *anim, ufbx_element *element, double time, ufbx_prop *buffer, size_t buffer_size);
ufbx_abi void ufbx_ffi_evaluate_transform(ufbx_transform *retval, const ufbx_anim *anim, const ufbx_node *node, double time);
ufbx_abi ufbx_real ufbx_ffi_evaluate_blend_weight(const ufbx_anim *anim, const ufbx_blend_channel *channel, double time);
ufbx_abi void ufbx_ffi_prepare_prop_overrides(ufbx_const_prop_override_list *retval, ufbx_prop_override *overrides, size_t num_overrides);
ufbx_abi void ufbx_ffi_quat_mul(ufbx_quat *retval, const ufbx_quat *a, const ufbx_quat *b);
ufbx_abi void ufbx_ffi_quat_normalize(ufbx_quat *retval, const ufbx_quat *q);
ufbx_abi void ufbx_ffi_quat_fix_antipodal(ufbx_quat *retval, const ufbx_quat *q, const ufbx_quat *reference);
ufbx_abi void ufbx_ffi_quat_slerp(ufbx_quat *retval, const ufbx_quat *a, const ufbx_quat *b, ufbx_real t);
ufbx_abi void ufbx_ffi_quat_rotate_vec3(ufbx_vec3 *retval, const ufbx_quat *q, const ufbx_vec3 *v);
ufbx_abi void ufbx_ffi_quat_to_euler(ufbx_vec3 *retval, const ufbx_quat *q, ufbx_rotation_order order);
ufbx_abi void ufbx_ffi_euler_to_quat(ufbx_quat *retval, const ufbx_vec3 *v, ufbx_rotation_order order);
ufbx_abi void ufbx_ffi_matrix_mul(ufbx_matrix *retval, const ufbx_matrix *a, const ufbx_matrix *b);
ufbx_abi void ufbx_ffi_matrix_invert(ufbx_matrix *retval, const ufbx_matrix *m);
ufbx_abi void ufbx_ffi_matrix_for_normals(ufbx_matrix *retval, const ufbx_matrix *m);
ufbx_abi void ufbx_ffi_transform_position(ufbx_vec3 *retval, const ufbx_matrix *m, const ufbx_vec3 *v);
ufbx_abi void ufbx_ffi_transform_direction(ufbx_vec3 *retval, const ufbx_matrix *m, const ufbx_vec3 *v);
ufbx_abi void ufbx_ffi_transform_to_matrix(ufbx_matrix *retval, const ufbx_transform *t);
ufbx_abi void ufbx_ffi_matrix_to_transform(ufbx_transform *retval, const ufbx_matrix *m);
ufbx_abi void ufbx_ffi_get_skin_vertex_matrix(ufbx_matrix *retval, const ufbx_skin_deformer *skin, size_t vertex, const ufbx_matrix *fallback);
ufbx_abi void ufbx_ffi_get_blend_shape_vertex_offset(ufbx_vec3 *retval, const ufbx_blend_shape *shape, size_t vertex);
ufbx_abi void ufbx_ffi_get_blend_vertex_offset(ufbx_vec3 *retval, const ufbx_blend_deformer *blend, size_t vertex);
ufbx_abi void ufbx_ffi_evaluate_nurbs_curve(ufbx_curve_point *retval, const ufbx_nurbs_curve *curve, ufbx_real u);
ufbx_abi void ufbx_ffi_evaluate_nurbs_surface(ufbx_surface_point *retval, const ufbx_nurbs_surface *surface, ufbx_real u, ufbx_real v);
ufbx_abi void ufbx_ffi_get_weighted_face_normal(ufbx_vec3 *retval, const ufbx_vertex_vec3 *positions, const ufbx_face *face);
ufbx_abi size_t ufbx_ffi_get_triangulate_face_num_indices(const ufbx_face *face);
ufbx_abi uint32_t ufbx_ffi_triangulate_face(uint32_t *indices, size_t num_indices, const ufbx_mesh *mesh, const ufbx_face *face);

ufbx_inline size_t ufbx_check_index(size_t index, size_t count) {
	(void)count;
	ufbx_assert(index < count);
	return index;
}

#define ufbx_at(list, index) ((list).data[ufbx_check_index((index), (list).count)])

#ifdef __cplusplus
}
#endif

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif

#endif

