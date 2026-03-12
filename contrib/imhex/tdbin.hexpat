#include <std/io.pat>
#include <std/mem.pat>
#include <std/sys.pat>
#include <std/math.pat>

#pragma pattern_limit 131072
#pragma array_limit 65536

struct SmallVec<T> {
    u8 size [[hidden]];
    T data[size] [[inline]];
};

struct Vec<T> {
    u32 size;
    T data[size] [[inline]];
};

struct Vec3 {
    float x, y, z;
} [[static, sealed, format("format_vec")]];

fn format_vec(ref auto vec) {
    return std::format("Vec({}, {}, {})", vec.x, vec.y, vec.z);
};

struct Quat {
    float x, y, z, w;
} [[static]];

struct Transform {
    Vec3 pos;
    Quat rot;
} [[static, sealed, format("format_tr")]];

fn format_tr(ref auto tr) {
    return std::format("Tr(Vec({}, {}, {}), Quat({}, {}, {}, {}))", tr.pos.x, tr.pos.y, tr.pos.z, tr.rot.x, tr.rot.y, tr.rot.z, tr.rot.w);
};

struct RGBA {
    float r, g, b, a;
} [[static, sealed, format("format_color")]];

fn format_color(ref auto color) {
    return std::format("{}, {}, {}, {}", color.r, color.g, color.b, color.a);
};

struct Vec2 {       // vertex
    float pos[2];   // pos
} [[static]];

struct Registry {
    char key[];
    char value[];
};

struct Tag {
    char name[];
    char value[];
};

struct Sound {
    char path[];
    float volume;
};

struct PostProcessing {
    float brightness;       // brightness
    RGBA colorbalance;      // colorbalance
    float saturation;       // saturation
    float gamma;            // gamma
    float bloom;            // bloom
} [[static, single_color]];

struct Player {
    Transform transform;
    float pitch;
    float yaw;
	Quat orientation;
	Quat camera_orientation;
    Vec3 velocity;
    float health;
    float transition_timer;
    float time_underwater;
    float bluetide_timer;
    float bluetide_power;
    u32 animator;
} [[static, single_color]];

struct Sun {
    Vec3 tint_brightness;
    RGBA colortint;     // sunColorTint
    Vec3 dir;           // sunDir
    float brightness;   // sunBrightness
    float spread;       // sunSpread
    float length;       // sunLength
    float fogscale;     // sunFogScale
    float glare;        // sunGlare
} [[static]];

struct Skybox {
    char texture[];         // skybox
    RGBA tint;             // skyboxtint
    float brightness;       // skyboxbrightness
    float rot;             // skyboxrot in radians
    Sun sun;                // sun
    bool auto_sun_dir;
    RGBA constant;         // constant
    float ambient;         // ambient
    float ambientexponent;  // ambientexponent
};

struct Fog {
    u8 type;
    RGBA color;         // fogColor
    // fogParams:
    float start;
    float distance;     // end = start + distance
    float amount;
    float exponent;
    float height_offset;
} [[static]];

struct EnvWater {
    float wetness;      // wetness
    float puddleamount; // puddleamount
    float puddlesize;   // puddlesize =  0.01 / this
    float rain;         // rain
} [[static]];

struct Snow {
    Vec3 dir;       // snowdir
    float spread;   // snowdir spread
    float amount;   // snowamount
    float speed;
    bool onground;  // snowonground
} [[static]];

struct Environment {
    Skybox skybox;      // skybox
    float exposure[2];  // exposure
    float brightness;   // brightness
    Fog fog;            // fog
    EnvWater water;     // water
    bool nightlight;    // nightlight
    Sound ambience;     // ambience
    float slippery;     // slippery
    float fogscale;     // fogscale
    Snow snow;         // snow
    Vec3 wind;         // wind
    float waterhurt;    // waterhurt
    char lensdirt[];    // lensdirt
} [[single_color]];

struct Boundary {
    Vec<Vec2> vertices;
    float padleft;      // -padleft
    float padtop;       // -padtop
    float padright;     // padright
    float padbottom;    // padbottom
    float maxheight;    // maxheight
} [[single_color]];

struct Fire {
    u32 shape;
    Vec3 position;
    float max_time;
    float time;
    bool painted;
    bool broken;
    u32 spawned_count;
} [[static]];

enum MaterialType : u8 {
    None,
    Glass,
    Wood,
    Masonry,
    Plaster,
    Metal,
    HeavyMetal,
    Rock,
    Dirt,
    Foliage,
    Plastic,
    HardMetal,
    HardMasonry,
    Ice,
    Unphysical
};

struct Material {
    MaterialType type;
    RGBA RGBA;
    float reflectivity;
    float shinyness;
    float metalness;
    float emissive;
    bool is_tint;
} [[static]];

struct Palette {
    Material materials[256];
    bool has_transparent;
    u8 black_tint[4 * 256];
    u8 yellow_tint[4 * 256];
    u8 rgba_tint[4 * 256];
} [[static, single_color]];

enum EntityType : u8 {
    Body = 1,
    Shape,
    Light,
    Location,
    Water,
    Joint,
    Vehicle,
    Wheel,
    Screen,
    Trigger,
    Script,
    Animator,
};

enum BodyMode : u8 {
    Average,
    Minimum,
    Multiply,
    Maximum,
};

struct Body {
    u16 flags;
    Transform transform;
    Vec3 velocity;
    Vec3 angular_velocity;
    bool dynamic;           // dynamic
    u8 body_flags;
    float friction;
    BodyMode friction_mode;
    float restitution;
    BodyMode restitution_mode;
} [[static]];

enum ShapeOrigin : u8 {
    Tool = 1,
    MapInit,
    Debris,
    CreateShape,
    Spawn,
};

struct Voxels {
    u32 sizex;
    u32 sizey;
    u32 sizez;
    if (sizex * sizey * sizez > 0)
        Vec<u8> palette_indexes;
    u32 palette;                // palette
    float scale;                // scale = 10.0 * this
    u8 light_mask[8];           // - - - - | f r b -
    bool is_disconnected;
};

struct Shape {
    u16 flags;
    Transform transform;
    u16 shape_flags;            // collide: 0x10
    u8 collision_layer;
    u8 collision_mask;
    float density;             // density
    float strength;           // strength
    u16 texture_tile;           // texture
    u16 blendtexture_tile;      // blendtexture
    float texture_weight;       // texture
    float blendtexture_weight;  // blendtexture
    Vec3 texture_offset;
    float emissive_scale;
    u8 is_broken;
    u8 has_voxels;
    Voxels voxels;             // voxels
    ShapeOrigin origin;
    u32 z_u32;
};

enum LightType : u8 {
    Sphere = 1,
    Capsule,
    Cone,
    Area,
};

struct Light {
    bool is_on;
    LightType type;     // type
    Transform transform;
    RGBA color;         // color = pow(this, 0.454545)
    float scale;        // scale
    float reach;        // reach
    float size;         // size
    float unshadowed;   // unshadowed
    float angle;        // angle = 2.0 * deg(acos(this))
    float penumbra;     // penumbra = 2.0 * deg(acos(angle) - acos(this))
    float fogiter;      // fogiter
    float fogscale;     // fogscale
    float area_size[2]; // size = 2.0 * this
    float capsule_size; // size.x = 2.0 * this
    Vec3 position;
    u8 index;
    float flickering;
    Sound sound;        // sound
    float glare;        // glare
} [[static]];

struct Location {
    u16 flags;
    Transform transform;
} [[static]];

struct Water {
    u16 flags;
    Transform transform;
    float depth;        // depth
    float wave;         // wave
    float ripple;       // ripple
    float motion;       // motion
    float foam;         // foam
    RGBA color;         // color
    float visibility;   // visibility
    Vec<Vec2> vertices; // vertices
};

struct Segment {
    Vec3 from_;
    Vec3 to;
} [[static]];

struct Rope {
    RGBA color;     // color
    float zero;
    float strength;     // strength
    float maxstretch;   // maxstretch
    float slack;        // slack = this - length
    float segment_length;
    u8 active;
    Vec<Segment> segments;
};

enum JointType : u32 {
    Ball = 1,
    Hinge,
    Prismatic,
    Rope,
    Cone,
};

struct Joint {
    JointType type;           // type
    u32 shapes[2];
    Vec3 positions[2];
    Vec3 axis[2];
    bool connected;
    bool collide;               // collide
    float rotstrength;         // rotstrength
    float rotspring;            // rotspring
    Quat hinge_rot;
    float limits[2];            // limits (in degrees for hinge, meters for prismatic)
    float max_velocity;
    float strength;
    float size;             // size
    bool sound;             // sound
    bool autodisable;           // autodisable
    float connection_strength;
    float disconnect_dist;
    if (type == JointType::Rope)
        Rope rope;
};

struct VehicleSound {
    char name[];
    float pitch;
};

struct VehicleProperties {
    float topspeed;     // topspeed = 3.6 * this
    float top_speed_clamp;
    float spring;       // spring
    float damping;      // damping
    float acceleration; // acceleration
    float strength;     // strength
    float friction;     // friction
    float max_steering_angle;
    bool handbrake;
    float antispin;     // antispin
    float steerassist;  // steerassist
    float assist_multiplier;
    float antiroll;     // antiroll
    VehicleSound sound; // sound
};

struct Exhaust {
    Transform transform;
    float strength;
} [[static]];

struct Vital {
    u32 body;
    Vec3 position;
    float radius;
    u32 nearby_voxels;
} [[static]];

struct VehicleLocation {
    char name[];
    Transform transform;
    u32 handle;
};

struct Vehicle {
    u16 flags;
    u32 body;
    Transform transform;
    Transform transform2;
    Vec<u32> wheels;
    VehicleProperties properties;
    Vec3 camera;            // camera
    Vec3 player;            // player
    Vec3 exit;             // exit
    Vec3 propeller;         // propeller
    float difflock;         // difflock
    float health;
    u32 body_voxel_count;
    bool braking;
    float passive_brake;
    Vec<u32> refs;
    Vec<Exhaust> exhausts;  // exhaust
    Vec<Vital> vitals;      // vital
    Vec<VehicleLocation> locations;
    float bounds_dist;
    bool noroll;
    float brokenthreshold;
    float smokeintensity;
};

struct Wheel {
    u16 flags;
    u32 vehicle;
    u32 vehicle_body;
    u32 body;
    u32 shape;
    u32 ground_shape;
    u32 ground_voxel_pos[3];
    bool on_ground;
    Transform transform;
    Transform transform2;
    float steer;            // steer
    float drive;            // drive
    float travel[2];        // travel
    float radius;
    float width;
    float angular_speed;
    float stance;
    float vertical_offset;
} [[static]];

struct Screen {
    u16 flags;
    Transform transform;
    float size[2];         // size
    float bulge;            // bulge
    u32 resolution[2];      // resolution
    char script[];         // script
    bool enabled;           // enabled
    bool interactive;       // interactive
    float emissive;         // emissive
    float fxraster;         // fxraster
    float fxca;           // fxca
    float fxnoise;         // fxnoise
    float fxglitch;
};

enum TriggerType : u32 {
    Sphere = 1,
    Box,
    Polygon,
};

struct TriggerSound {
    char path[];        // sound
    float ramp;         // soundramp
    u8 type;
    float volume;       // sound
};

struct Trigger {
    u16 flags;
    Transform transform;
    TriggerType type;           // type
    float sphere_size;         // size
    Vec3 box_size;             // size = 2.0 * this
    float polygon_size;         // size
    Vec<Vec2> polygon_vertices;
    TriggerSound sound;
};

enum SoundType : u32 {
    Sound = 1,
    Loop = 2/*,
    Music = ?,
    UiSound = ?
    UiLoop = ?*/
};

struct ScriptSound {
    SoundType type;
    char name[];
};

enum TransitionType : u8 {
    Linear = 1,
    EaseIn = 2,
    EaseOut = 3,
    Cosine = 4,
    Bounce = 5
};

struct ValueTransition {
    char variable[];
    TransitionType transition;
    float target_time;
    float current_time;
    float current_value;
    float target_value;
};

using LuaTable;

enum LuaType : u32 {
    NIL = 0,
    Boolean = 1,
    Number = 3,
    String = 4,
    Table = 5,
    Reference = 0xFFFFFFFB
};

struct LuaTableEntry {
    LuaType key_type;
    if (key_type != LuaType::NIL) {
        if (key_type == LuaType::Boolean)
            bool key;
        else if (key_type == LuaType::Number)
            double key;
        else if (key_type == LuaType::String)
            char key[];
        else if (key_type == LuaType::Table)
            LuaTable key;
        else if (key_type == LuaType::Reference)
            u32 key;

        LuaType value_type;
        if (value_type == LuaType::Boolean)
            bool value;
        else if (value_type == LuaType::Number)
            double value;
        else if (value_type == LuaType::String)
            char value[];
        else if (value_type == LuaType::Table)
            LuaTable value;
        else if (value_type == LuaType::Reference)
            u32 value;
    }
};

struct LuaTable {
    LuaTableEntry entries[while (std::mem::read_unsigned($, 4) != LuaType::NIL)];
    u32 end;
};

struct Script {
    u16 flags;
    char file[];            // file
    Vec<Registry> params;   // param%d

    float tick_time;
    float update_time;
    u32 variables_count;
    LuaTable table;
    Vec<u32> entities;
    Vec<ScriptSound> sounds;
    Vec<ValueTransition> transitions;
};

struct Bone {
    u32 body;
    s32 index;
    s32 parent_index;
    char name[];
};

struct Keyframe {
    Transform transform;
    float time;
} [[static]];

struct Sequence {
    u32 bone_index;
    Vec<Keyframe> frames;
};

struct Animation {
    char name[];
    u32 start_bone;
    float anim_rate;
    Vec<Sequence> seqs;
};

struct AnimListB {
    Transform transform;
    Vec2 v1;
    Vec2 v2;
    float z_f32_1;
    float z_f32_2;
    u8 z_u8_1;
    u8 z_u8_2;
    u32 handle_1;
    u32 handle_2;
    u32 handle_3;
    u32 handle_4;
    Quat rotation;
    Vec3 z_v3_1;
    Vec3 z_v3_2;
    Vec3 z_v3_3;
    Vec3 z_v3_4;
    s32 size_1;
    s32 size_2;
};

struct AnimListC {
    u32 size;
    Transform transform;
    float z_f32_1;
    float z_f32_2;
    u32 handle_1;
    u32 handle_2;
    u8 z_u8_4[4];
    bool has_voxels;
    Voxels voxels;
};

struct AnimListD {
    char str1[];
    u8 array[56];
};

struct AnimListE {
    char str1[];
    u8 array[128];
};

struct AnimListF {
    char str1[];
    u8 array[72];
};

struct AnimListG {
    char str1[];
};

struct AnimListH {
    char str1[];
    Transform transform;
};

struct Animator {
    u16 flags;

    Transform transform;
    char path[];

    bool is_ragdoll;

    Vec<Bone> skeleton;
    Vec<AnimListB> vec_b;
    Vec<AnimListC> vec_c;
    Vec<AnimListD> vec_d;
    Vec<AnimListE> vec_e;   // Animation Instances

    u32 z_size1;

    Vec<AnimListF> vec_f;   // Playing instances

    u32 z_size2;

    u32 mul_1;
    u8 array1[8 * mul_1];   // Animation Group

    Vec<AnimListG> vec_g;

    u32 mul_2;
    u8 array2[28 * mul_2];

    u32 mul_3;
    u8 array3[28 * mul_3];

    u32 mul_4;
    u8 array4[4 * mul_4];

    Vec<AnimListH> vec_h;   // IK Transform
};

using Entity;

struct Entity {
    EntityType type;
    u32 handle;
    SmallVec<Tag> tags; // tags
    char desc[];        // desc
    if (type == EntityType::Body)
        Body body;
    else if (type == EntityType::Shape)
        Shape shape;
    else if (type == EntityType::Light)
        Light light;
    else if (type == EntityType::Location)
        Location location;
    else if (type == EntityType::Water)
        Water water;
    else if (type == EntityType::Joint)
        Joint joint;
    else if (type == EntityType::Vehicle)
        Vehicle vehicle;
    else if (type == EntityType::Wheel)
        Wheel wheel;
    else if (type == EntityType::Screen)
        Screen screen;
    else if (type == EntityType::Trigger)
        Trigger trigger;
    else if (type == EntityType::Script)
        Script script;
    else if (type == EntityType::Animator)
        Animator animator;
    else
        std::assert(true, "Invalid entity type");
    Vec<Entity> children;
    u32 beef_beef;
    std::assert(beef_beef == 0xBEEFBEEF, "Invalid cow");
} [[single_color]];

struct Projectile {
    Vec3 origin;
    Vec3 direction;
    float dist;
    float max_dist;
    u32 type;
    float strength;
} [[static, single_color]];

struct Path {
    char folder[];
    bool do_override;
} [[single_color]];

struct Scene {
    char magic[5];
    u8 version[3] [[sealed, format("format_version")]];
    char level_id[];
    char level_path[];
    char layers[];
    char mod[];

    u32 aaa1;
    Vec<Registry> enabled_mods [[single_color]];
    Vec<Registry> spawn_mods [[single_color]];

    u32 driven_vehicle;
    Vec3 shadow_volume;
    Vec3 gravity;
    Transform spawnpoint;
    u32 world_body;
    u32 flashlight;
    u32 explosion_lua;
    u32 achievements_lua;
    u32 character_lua;
    PostProcessing postpro;
    Player player;
    Environment environment;
    Boundary boundary;
    Vec<Fire> fires [[single_color]];
    Vec<Palette> palettes [[single_color]];
    Vec<Registry> registry [[single_color]];
    Vec<Entity> entities;
    u32 entity_count;
    u32 broken_voxels;
    Vec<Projectile> projectiles;
    bool has_snow;
    Vec<Path> assets;
} [[inline]];

fn format_version(ref auto version) {
    return std::format("{}.{}.{}", version[0], version[1], version[2]);
};

Scene scene @ 0x00;
