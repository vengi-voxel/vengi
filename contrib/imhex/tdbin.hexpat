#include <std/io.pat>
#include <std/mem.pat>
#include <std/sys.pat>
#include <std/math.pat>

#pragma array_limit 65536
#pragma pattern_limit 131072

struct SmallVec<T> {
    u8 size [[hidden]];
    T data[size] [[inline]];
};

struct MediumVec<T> {
    u16 size [[hidden]];
    T data[size] [[inline]];
};

struct Vec<T> {
    u32 size [[hidden]];
    T data[size] [[inline]];
};

struct Vec2 {
    float x;
    float y;
} [[static, sealed]];

struct Vec3 {
    float x, y, z;
} [[static, sealed, format("format_vec")]];

fn format_vec(ref auto vec) {
    return std::format("Vec({}, {}, {})", vec.x, vec.y, vec.z);
};

struct Quat {
    float x, y, z, w;
} [[static, sealed, format("format_quat")]];

fn format_quat(ref auto quat) {
    return std::format("Quat({}, {}, {}, {})", quat.x, quat.y, quat.z, quat.w);
};

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

struct Registry {
    char key[];
    char value[];
    bool sync;
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
    float brightness;
    RGBA colorbalance;
    float saturation;
    float gamma;
    float bloom;
} [[static, single_color]];

struct Sun {
    Vec3 tint_brightness;
    RGBA colortint;
    Vec3 dir;
    float brightness;
    float spread;
    float length;
    float fogscale;
    float glare;
} [[static]];

struct Skybox {
    char texture[];
    RGBA tint;
    float brightness;
    float rot;
    Sun sun;
    bool auto_sun_dir;
    RGBA constant;
    float ambient;
    float ambientexponent;
};

struct Fog {
    u8 type;
    RGBA color;
    float start;
    float distance;
    float amount;
    float exponent;
    float height_offset;
} [[static]];

struct EnvWater {
    float wetness;
    float puddleamount;
    float puddlesize;
    float rain;
} [[static]];

struct Snow {
    Vec3 dir;
    float spread;
    float amount;
    float speed;
    bool onground;
} [[static]];

struct Environment {
    Skybox skybox;
    float exposure[2];
    float brightness;
    Fog fog;
    EnvWater water;
    bool nightlight;
    Sound ambience;
    float slippery;
    float fogscale;
    Snow snow;
    Vec3 wind;
    float waterhurt;
    char lensdirt[];
} [[single_color]];

struct Boundary {
    Vec<Vec2> vertices;
    float padleft;
    float padtop;
    float padright;
    float padbottom;
    float maxheight;
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
    Rig,
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
    bool dynamic;
    u8 body_flags;
    float friction;
    BodyMode friction_mode;
    float restitution;
    BodyMode restitution_mode;
} [[static]];

struct Voxels {
    u32 sizex;
    u32 sizey;
    u32 sizez;
    if (sizex * sizey * sizez > 0)
        Vec<u8> palette_indexes;
    u32 palette;
    float scale;
    u8 light_mask[8];
    bool is_disconnected;
};

struct Shape {
    u16 flags;
    Transform transform;
    u16 shape_flags;
    u8 collision_layer;
    u8 collision_mask;
    float density;
    float strength;
    u16 texture_tile;
    u16 blendtexture_tile;
    float texture_weight;
    float blendtexture_weight;
    Vec3 texture_offset;
    float emissive_scale;
    u8 is_broken;
    u8 has_voxels;
    Voxels voxels;
    u8 origin;
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
    LightType type;
    Transform transform;
    RGBA color;
    float scale;
    float reach;
    float size;
    float unshadowed;
    float angle;
    float penumbra;
    float fogiter;
    float fogscale;
    float area_size[2];
    float capsule_size;
    Vec3 position;
    u8 index;
    float flickering;
    Sound sound;
    float glare;
    char breaksound[];
} [[static]];

struct Location {
    u16 flags;
    Transform transform;
} [[static]];

struct Water {
    u16 flags;
    Transform transform;
    float depth;
    float wave;
    float ripple;
    float motion;
    float foam;
    RGBA color;

    float unk1[5];
    float drag;
    u8 unk2;
    float unk3[25];
    u8 unk6;
    float visibility;
    Vec<Vec2> vertices;
};

struct Segment {
    Vec3 from_;
    Vec3 to;
} [[static]];

struct Rope {
    RGBA color;
    float zero;
    float strength;
    float maxstretch;
    float slack;
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
    JointType type;
    u32 shapes[2];
    Vec3 positions[2];
    Vec3 axis[2];
    bool connected;
    bool collide;
    float rotstrength;
    float rotspring;
    Quat hinge_rot;
    float limits[2];
    float max_velocity;
    float strength;
    float size;
    bool sound;
    bool autodisable;
    float connection_strength;
    float disconnect_dist;
    if (type == JointType::Rope)
        Rope rope;
};

struct VehicleProperties {
    float topspeed;
    float top_speed_clamp;
    float spring;
    float damping;
    float acceleration;
    float strength;
    float friction;
    float max_steering_angle;
    bool handbrake;
    float antispin;
    float steerassist;
    float assist_multiplier;
    float antiroll;
    Sound sound;
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

struct VehiclePassenger {
    s32 rig;
    u32 unk1[2];
    bool unk2;
};

struct Vehicle {
    u16 flags;
    u32 body;
    Transform transform;
    Transform transform2;
    Vec<u32> wheels;
    VehicleProperties properties;
    Vec3 camera;
    Vec3 player;
    Vec3 driver_seat;
    Vec3 exit;
    Vec3 propeller;
    float difflock;
    float health;
    u32 main_voxel_count;
    bool braking;
    float passive_brake;
    Vec<u32> bodies;
    Vec<Exhaust> exhausts;
    Vec<Vital> vitals;
    Vec<VehicleLocation> locations;
    Vec<VehiclePassenger> passengers;
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
    float steer;
    float drive;
    float travel[2];
    float radius;
    float width;
    float angular_speed;
    float stance;
    float vertical_offset;
} [[static]];

struct Screen {
    u16 flags;
    Transform transform;
    float size[2];
    float bulge;
    u32 resolution[2];
    char script[];
    bool enabled;
    bool interactive;
    float emissive;
    float fxraster;
    float fxca;
    float fxnoise;
    float fxglitch;
};

enum TriggerType : u32 {
    Sphere = 1,
    Box,
    Polygon,
};

struct TriggerSound {
    char path[];
    float ramp;
    u8 type;
    float volume;
};

struct Trigger {
    u16 flags;
    Transform transform;
    TriggerType type;
    float sphere_size;
    Vec3 box_size;
    float polygon_size;
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
    char path[];
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
    }
    LuaType value_type;
    if (value_type != LuaType::NIL) {
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

struct ScriptSprite {
    u32 type;
    char path[];
};

struct IntPair {
    u32 first;
    u32 second;
};

struct ScriptCore {
	Vec<Tag> params;
    float tick_time;
    float update_time;
    bool unk1;
    bool unk2;
    u32 variables_count;
    LuaTable table;
    Vec<u32> entities;
    Vec<ScriptSound> sounds;
    Vec<ValueTransition> transitions;
    Vec<ScriptSprite> unk3;
    MediumVec<IntPair> unk4;
};

struct Script {
    u16 flags;
    u32 unk_size;
    char file[];
    char unk_path[];
    bool unk_bool1;
    bool unk_bool2;
    bool is_server;
    if (is_server)
        ScriptCore server_core;
    ScriptCore client_core;
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

struct RigLoc {
    char name[];
    Transform transform;
    bool unk1;
};

struct Rig {
    u16 flags;
    Vec<RigLoc> locations;
    Transform transform;
    bool unk1;
    s32 vehicle;
    u32 unk3;
};

using Entity;

struct Entity {
    EntityType type;
    s32 handle;
    SmallVec<Tag> tags;
    char desc[];
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
    else if (type == EntityType::Rig)
        Rig rig;
    else
        std::assert(true, "Invalid entity type");
    Vec<Entity> children;
    u32 beef_beef;
    std::assert(beef_beef == 0xBEEFBEEF, "Invalid cow");
} [[single_color]];

struct Path {
    char folder[];
    bool do_override;
} [[single_color]];

struct ToolInfo {
    bool enabled;
    char id[];
    char name[];
    Transform transform;
    float ammo_pickup_amount;
    if (ammo_pickup_amount > 0)
        u32 ammo;
};

struct ToolInfoExtended {
    ToolInfo base;
    char path[];
    char file[];
    u32 group;
};

struct Player {
    Transform transform;
    float pitch;
    float yaw;
    Quat orientation;
    Quat camera_orientation;
    Vec3 velocity;
    float health;
    float bluetide_timer; // wrong???
    float time_underwater;
    s32 driven_vehicle;
    s32 flashlight;
    s32 flashlight2;
    u32 unk1;
    u32 unk2;
    float unk3; // bluetide_timer???
    float unk4;
    s32 animator;
    u32 unk5;
    s32 animator2;
    ToolInfo tools_info[17];
    Vec<ToolInfoExtended> mod_tools_info;
    char current_tool[];
} [[single_color]];

struct Projectile {
    Vec3 origin;
    Vec3 direction;
    float dist;
    float max_dist;
    float strength;
    u32 type;
    u32 player_id;
    bool impact;
} [[static]];

struct Scene {
    char magic[5];
    u8 version[3] [[sealed, format("format_version")]];
    char level_id[];
    char level_path[];
    char layers[];
    char mod[];

    u32 unk1; // (?) value:
    u32 aaa1;
    Vec<Tag> active_mods [[single_color]];
    Vec<Tag> spawn_mods [[single_color]];

    Vec3 shadow_volume;
    Vec3 gravity;
    Transform spawnpoint;

    u32 world_body;
    u32 explosionclient_lua;
    u32 characters_lua;
    u32 achievements_lua;

    PostProcessing postpro;
    Vec<u32> player_ids;
    Player player[player_ids.size];
    Environment environment;
    Boundary boundary;
    Vec<Projectile> projectiles [[single_color]];
    Vec<Fire> fires [[single_color]];

    u32 unk2; // (?) vec<{int, int}> (entities) player rig, <id, rig>
    Vec<Palette> palettes [[single_color]];
    Vec<Registry> registry [[single_color]];
    Vec<Entity> entities;
    // ...
} [[inline]];

fn format_version(ref auto version) {
    return std::format("{}.{}.{}", version[0], version[1], version[2]);
};

Scene scene @ 0x00;
