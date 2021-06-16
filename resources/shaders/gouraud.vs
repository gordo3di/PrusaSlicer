#version 110

#define INTENSITY_CORRECTION 0.6

// normalized values for (-0.6/1.31, 0.6/1.31, 1./1.31)
const vec3 LIGHT_TOP_DIR = vec3(-0.4574957, 0.4574957, 0.7624929);
#define LIGHT_TOP_DIFFUSE    (0.8 * INTENSITY_CORRECTION)
#define LIGHT_TOP_SPECULAR   (0.125 * INTENSITY_CORRECTION)
#define LIGHT_TOP_SHININESS  20.0

// normalized values for (1./1.43, 0.2/1.43, 1./1.43)
const vec3 LIGHT_FRONT_DIR = vec3(0.6985074, 0.1397015, 0.6985074);
#define LIGHT_FRONT_DIFFUSE  (0.3 * INTENSITY_CORRECTION)
//#define LIGHT_FRONT_SPECULAR (0.0 * INTENSITY_CORRECTION)
//#define LIGHT_FRONT_SHININESS 5.0

#define INTENSITY_AMBIENT    0.3

#define PI 3.1415926538
#define TWO_PI (2.0 * PI)

const vec3 ZERO = vec3(0.0, 0.0, 0.0);

struct PrintBoxDetection
{
    bool active;
    vec3 min;
    vec3 max;
    mat4 volume_world_matrix;
};

struct SlopeDetection
{
    bool active;
	float normal_z;
    mat3 volume_world_normal_matrix;
};

struct BoundingBox
{
    vec3 center;
    vec3 sizes;
};

struct ProjectedTexture
{
    bool active;
    // 0 = cubic, 1 = cylindrical, 2 = spherical
    int projection;
    BoundingBox box;
};

struct ClippingPlane
{
    bool active;
    // Clipping plane, x = min z, y = max z. Used by the FFF and SLA previews to clip with a top / bottom plane.
    vec2 z_range;
    // Clipping plane - general orientation. Used by the SLA gizmo.
    vec4 plane;
};

uniform PrintBoxDetection print_box;
uniform SlopeDetection slope;
uniform ProjectedTexture proj_texture;
uniform ClippingPlane clipping_plane;

// x = diffuse, y = specular;
varying vec2 intensity;

varying vec3 delta_box_min;
varying vec3 delta_box_max;

varying vec3 clipping_planes_dots;

varying vec4 model_pos;
varying float world_pos_z;
varying float world_normal_z;
varying vec3 eye_normal;

varying vec2 tex_coords;

vec2 calc_intensity(vec3 eye_position, vec3 eye_normal)
{
    vec2 ret = vec2(0.0, 0.0);
    
    // Compute the cos of the angle between the normal and lights direction. The light is directional so the direction is constant for every vertex.
    // Since these two are normalized the cosine is the dot product. We also need to clamp the result to the [0,1] range.
    float NdotL = max(dot(eye_normal, LIGHT_TOP_DIR), 0.0);

    ret.x = INTENSITY_AMBIENT + NdotL * LIGHT_TOP_DIFFUSE;
    ret.y = LIGHT_TOP_SPECULAR * pow(max(dot(-normalize(eye_position), reflect(-LIGHT_TOP_DIR, eye_normal)), 0.0), LIGHT_TOP_SHININESS);

    // Perform the same lighting calculation for the 2nd light source (no specular applied).
    NdotL = max(dot(eye_normal, LIGHT_FRONT_DIR), 0.0);
    ret.x += NdotL * LIGHT_FRONT_DIFFUSE;
    
    return ret;
}

float azimuth(vec2 dir)
{
    float ret = atan(dir.y, dir.x); // [-PI..PI]
    if (ret < 0.0)
        ret += TWO_PI; // [0..2*PI]
    ret /= TWO_PI; // [0..1]    
    return ret;
}

vec2 cubic_uv(vec3 position)
{
    vec2 ret = vec2(0.0, 0.0);
    return ret;
}

vec2 cylindrical_uv(vec3 position, vec3 normal)
{
    vec2 ret = vec2(0.0, 0.0);
    vec3 dir = position - proj_texture.box.center;
    if (length(normal.xy) == 0.0) {
        // caps
        ret = dir.xy / proj_texture.box.sizes.xy + 0.5;
        if (dir.z < 0.0)
            ret.y = 1.0 - ret.y;
    }
    else {
        ret.x = azimuth(dir.xy);        
        float min_z = proj_texture.box.center.z - 0.5 * proj_texture.box.sizes.z;
        ret.y = (position.z - min_z) / proj_texture.box.sizes.z; // [0..1]
    }
    return ret;
}

vec2 spherical_uv(vec3 position)
{
    vec2 ret = vec2(0.0, 0.0);
    vec3 dir = position - proj_texture.box.center;
    ret.x = azimuth(dir.xy);
    ret.y = atan(length(dir.xy), -dir.z) / PI; // [0..1]
    return ret;
}

void main()
{
    // Transform the position into camera space.
    vec4 eye_position = gl_ModelViewMatrix * gl_Vertex;
    // Transform the normal into camera space and normalize the result.
    eye_normal = normalize(gl_NormalMatrix * gl_Normal);
    
    intensity = calc_intensity(eye_position.xyz, eye_normal);

    model_pos = gl_Vertex;
    // Point in homogenous coordinates.
    vec4 world_pos = print_box.volume_world_matrix * gl_Vertex;
    world_pos_z = world_pos.z;

    // compute deltas for out of print volume detection (world coordinates)
    if (print_box.active) {
        delta_box_min = world_pos.xyz - print_box.min;
        delta_box_max = world_pos.xyz - print_box.max;
    }
    else {
        delta_box_min = ZERO;
        delta_box_max = ZERO;
    }

    // z component of normal vector in world coordinate used for slope shading
	world_normal_z = slope.active ? (normalize(slope.volume_world_normal_matrix * gl_Normal)).z : 0.0;

    gl_Position = ftransform();
    
    // Fill in the scalars for fragment shader clipping. Fragments with any of these components lower than zero are discarded.
    if (clipping_plane.active)
        clipping_planes_dots = vec3(dot(world_pos, clipping_plane.plane), world_pos.z - clipping_plane.z_range.x, clipping_plane.z_range.y - world_pos.z);
    
    if (proj_texture.active) {
        if (proj_texture.projection == 1)
            tex_coords = cylindrical_uv(gl_Vertex.xyz, gl_Normal);
        else if (proj_texture.projection == 2)
            tex_coords = spherical_uv(gl_Vertex.xyz);
        else
            tex_coords = cubic_uv(gl_Vertex.xyz);
    }
    else
        vec2(0.0);
}
