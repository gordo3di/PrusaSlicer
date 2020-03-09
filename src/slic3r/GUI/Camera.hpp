#ifndef slic3r_Camera_hpp_
#define slic3r_Camera_hpp_

#include "libslic3r/BoundingBox.hpp"
#if ENABLE_THUMBNAIL_GENERATOR
#include "3DScene.hpp"
#endif // ENABLE_THUMBNAIL_GENERATOR
#include <array>

namespace Slic3r {
namespace GUI {

struct Camera
{
    static const double DefaultDistance;
#if ENABLE_THUMBNAIL_GENERATOR
    static const double DefaultZoomToBoxMarginFactor;
    static const double DefaultZoomToVolumesMarginFactor;
#endif // ENABLE_THUMBNAIL_GENERATOR
    static double FrustrumMinZRange;
    static double FrustrumMinNearZ;
    static double FrustrumZMargin;
    static double MaxFovDeg;

    enum EType : unsigned char
    {
        Unknown,
        Ortho,
        Perspective,
        Num_types
    };

    bool requires_zoom_to_bed;

private:
    EType m_type;
    Vec3d m_target;
    float m_zenit;
    double m_zoom;
    // Distance between camera position and camera target measured along the camera Z axis
    mutable double m_distance;
    mutable double m_gui_scale;

    mutable std::array<int, 4> m_viewport;
    mutable Transform3d m_view_matrix;
    // We are calculating the rotation part of the m_view_matrix from m_view_rotation.
    mutable Eigen::Quaterniond m_view_rotation;
    mutable Transform3d m_projection_matrix;
    mutable std::pair<double, double> m_frustrum_zs;

    BoundingBoxf3 m_scene_box;

public:
    Camera();

    EType get_type() const { return m_type; }
    std::string get_type_as_string() const;
    void set_type(EType type);
    // valid values for type: "0" -> ortho, "1" -> perspective
    void set_type(const std::string& type);
    void select_next_type();

    const Vec3d& get_target() const { return m_target; }
    void set_target(const Vec3d& target);

    double get_distance() const { return (get_position() - m_target).norm(); }
    double get_gui_scale() const { return m_gui_scale; }

    double get_zoom() const { return m_zoom; }
    double get_inv_zoom() const { assert(m_zoom != 0.0); return 1.0 / m_zoom; }
    void update_zoom(double delta_zoom);
    void set_zoom(double zoom);

    const BoundingBoxf3& get_scene_box() const { return m_scene_box; }
    void set_scene_box(const BoundingBoxf3& box) { m_scene_box = box; }

    void select_view(const std::string& direction);

    const std::array<int, 4>& get_viewport() const { return m_viewport; }
    const Transform3d& get_view_matrix() const { return m_view_matrix; }
    const Transform3d& get_projection_matrix() const { return m_projection_matrix; }

    Vec3d get_dir_right() const { return m_view_matrix.matrix().block(0, 0, 3, 3).row(0); }
    Vec3d get_dir_up() const { return m_view_matrix.matrix().block(0, 0, 3, 3).row(1); }
    Vec3d get_dir_forward() const { return -m_view_matrix.matrix().block(0, 0, 3, 3).row(2); }

    Vec3d get_position() const { return m_view_matrix.matrix().inverse().block(0, 3, 3, 1); }

    double get_near_z() const { return m_frustrum_zs.first; }
    double get_far_z() const { return m_frustrum_zs.second; }

    double get_fov() const;

    void apply_viewport(int x, int y, unsigned int w, unsigned int h) const;
    void apply_view_matrix() const;
    // Calculates and applies the projection matrix tighting the frustrum z range around the given box.
    // If larger z span is needed, pass the desired values of near and far z (negative values are ignored)
    void apply_projection(const BoundingBoxf3& box, double near_z = -1.0, double far_z = -1.0) const;

#if ENABLE_THUMBNAIL_GENERATOR
    void zoom_to_box(const BoundingBoxf3& box, double margin_factor = DefaultZoomToBoxMarginFactor);
    void zoom_to_volumes(const GLVolumePtrs& volumes, double margin_factor = DefaultZoomToVolumesMarginFactor);
#else
    void zoom_to_box(const BoundingBoxf3& box, int canvas_w, int canvas_h);
#endif // ENABLE_THUMBNAIL_GENERATOR

#if ENABLE_CAMERA_STATISTICS
    void debug_render() const;
#endif // ENABLE_CAMERA_STATISTICS

    // translate the camera in world space
    void translate_world(const Vec3d& displacement) { this->set_target(m_target + displacement); }

    // rotate the camera on a sphere having center == m_target and radius == m_distance
    // using the given variations of spherical coordinates
    // if apply_limits == true the camera stops rotating when its forward vector is parallel to the world Z axis
    void rotate_on_sphere(double delta_azimut_rad, double delta_zenit_rad, bool apply_limits);

    // rotate the camera around three axes parallel to the camera local axes and passing through m_target
    void rotate_local_around_target(const Vec3d& rotation_rad);

    // returns true if the camera z axis (forward) is pointing in the negative direction of the world z axis
    bool is_looking_downward() const { return get_dir_forward().dot(Vec3d::UnitZ()) < 0.0; }

    void look_at(const Vec3d& position, const Vec3d& target, const Vec3d& up);

    double max_zoom() const { return 100.0; }
    double min_zoom() const;

private:
    // returns tight values for nearZ and farZ plane around the given bounding box
    // the camera MUST be outside of the bounding box in eye coordinate of the given box
    std::pair<double, double> calc_tight_frustrum_zs_around(const BoundingBoxf3& box) const;
#if ENABLE_THUMBNAIL_GENERATOR
    double calc_zoom_to_bounding_box_factor(const BoundingBoxf3& box, double margin_factor = DefaultZoomToBoxMarginFactor) const;
    double calc_zoom_to_volumes_factor(const GLVolumePtrs& volumes, Vec3d& center, double margin_factor = DefaultZoomToVolumesMarginFactor) const;
#else
    double calc_zoom_to_bounding_box_factor(const BoundingBoxf3& box, int canvas_w, int canvas_h) const;
#endif // ENABLE_THUMBNAIL_GENERATOR
    void set_distance(double distance) const;

    void set_default_orientation();
    Vec3d validate_target(const Vec3d& target) const;
    void update_zenit();
};

} // GUI
} // Slic3r

#endif // slic3r_Camera_hpp_

