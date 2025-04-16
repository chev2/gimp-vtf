// chev2/gimp-vtf - GIMP VTF file plugin
// Copyright (C) 2025  Chev <riskyrains@proton.me>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "vtfpp/ImageConversion.h"
#include "vtfpp/ImageFormats.h"
#include "vtfpp/VTF.h"
#include "file-vtf.h"
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

// Attribution constants
#define ATTRIBUTION_AUTHOR "Chev <riskyrains@proton.me>"
#define ATTRIBUTION_COPYRIGHT "GPL-3.0"
#define ATTRIBUTION_DATE "2025"

// Procedures prefixed with 'plug-in-chev' to avoid procedure name conflicts, like another VTF loading plugin
#define PROC_VTF_LOAD "plug-in-chev-file-vtf-load"
#define PROC_VTF_EXPORT "plug-in-chev-file-vtf-export"
#define PROC_VTF_BINARY "file-vtf"

struct _GimpVtf {
    GimpPlugIn parent_instance;
};

#define GIMP_VTF_TYPE (gimp_vtf_get_type())
G_DECLARE_FINAL_TYPE(GimpVtf, gimp_vtf, GIMP_VTF,, GimpPlugIn)
G_DEFINE_TYPE(GimpVtf, gimp_vtf, GIMP_TYPE_PLUG_IN)

static void gimp_vtf_class_init(GimpVtfClass *gclass) {
    GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS(gclass);

    // Called after every update of either GIMP or the plugin
    plug_in_class->query_procedures = gimp_vtf_query_procedures;

    // Returns interface of this procedure + metadata like title, menu path, author
    plug_in_class->create_procedure = gimp_vtf_create_procedure;
}

static void gimp_vtf_init(GimpVtf *gimp_vtf) {}

static GList *gimp_vtf_query_procedures(GimpPlugIn *plugin) {
    GList *list = NULL;
    
    list = g_list_append(list, g_strdup(PROC_VTF_LOAD));
    list = g_list_append(list, g_strdup(PROC_VTF_EXPORT));

    return list;
}

static GimpProcedure *gimp_vtf_create_procedure(GimpPlugIn *plugin, const gchar *name) {
    GimpProcedure *procedure = NULL;

    if (g_strcmp0(name, PROC_VTF_LOAD) == 0) {
        procedure = gimp_load_procedure_new(
            plugin, name, GIMP_PDB_PROC_TYPE_PLUGIN, gimp_vtf_load, NULL, NULL);
        // Only run when no image is open
        gimp_procedure_set_sensitivity_mask(procedure, GIMP_PROCEDURE_SENSITIVE_NO_IMAGE);
        gimp_procedure_set_menu_label(procedure, "VTF image");
        gimp_procedure_set_documentation(
            procedure,
            "Loads files in VTF file format",
            "This plug-in loads Valve Texture Format (VTF) files.",
            NULL
        );
        gimp_procedure_set_attribution(
            procedure,
            ATTRIBUTION_AUTHOR,
            ATTRIBUTION_COPYRIGHT,
            ATTRIBUTION_DATE
        );
        gimp_file_procedure_set_mime_types(GIMP_FILE_PROCEDURE(procedure), "image/x-vtf");
        gimp_file_procedure_set_extensions(GIMP_FILE_PROCEDURE(procedure), "vtf");
        gimp_file_procedure_set_magics(GIMP_FILE_PROCEDURE(procedure), "0,string,VTF\000");
    } else if (g_strcmp0(name, PROC_VTF_EXPORT) == 0) {
        procedure = gimp_export_procedure_new(
            plugin, name, GIMP_PDB_PROC_TYPE_PLUGIN, TRUE, gimp_vtf_export, NULL, NULL);
        //gimp_procedure_set_sensitivity_mask(procedure, GIMP_PROCEDURE_SENSITIVE_ALWAYS);
        gimp_procedure_set_image_types(procedure, "*");
        gimp_procedure_set_menu_label(procedure, "VTF image");
        gimp_procedure_set_documentation(
            procedure,
            "Exports files in VTF file format",
            "This plug-in exports Valve Texture Format (VTF) files.",
            NULL
        );
        gimp_procedure_set_attribution(
            procedure,
            ATTRIBUTION_AUTHOR,
            ATTRIBUTION_COPYRIGHT,
            ATTRIBUTION_DATE
        );
        gimp_file_procedure_set_format_name(GIMP_FILE_PROCEDURE(procedure), "VTF");
        gimp_file_procedure_set_mime_types(GIMP_FILE_PROCEDURE(procedure), "image/x-vtf");
        gimp_file_procedure_set_extensions(GIMP_FILE_PROCEDURE(procedure), "vtf");
        gimp_export_procedure_set_capabilities(
            GIMP_EXPORT_PROCEDURE(procedure),
            (GimpExportCapabilities)(
                GIMP_EXPORT_CAN_HANDLE_RGB
                | GIMP_EXPORT_CAN_HANDLE_ALPHA
                | GIMP_EXPORT_CAN_HANDLE_GRAY
                | GIMP_EXPORT_CAN_HANDLE_INDEXED
                | GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION
            ),
            NULL, NULL, NULL
        );
        
        //
        // VTF export arguments
        //

        // TODO: If the current image was an imported VTF, copy its settings here.

        // Version (7.0-7.6), default 7.4
        // 7.4 is what vtfpp uses by default; it's also the last version that most Source games support,
        //  causing breakage in a lot of games in 7.5 and beyond
        //  Source: https://developer.valvesoftware.com/wiki/VTF_(Valve_Texture_Format)#Versions
        GimpChoice *choice_version = gimp_choice_new_with_values(
            "7_0", 0, "7.0", NULL,
            "7_1", 1, "7.1", NULL,
            "7_2", 2, "7.2", NULL,
            "7_3", 3, "7.3", NULL,
            "7_4", 4, "7.4", NULL,
            "7_5", 5, "7.5", NULL,
            "7_6", 6, "7.6", NULL,
            NULL
        );
        gimp_procedure_add_choice_argument(
            procedure,
            "version",
            "VTF version",
            "VTF file version (7.0 to 7.6)."
            "\nRecommended: Use 7.4 for best compatibility.",
            choice_version,
            "7_4",
            G_PARAM_READWRITE
        );

        // Image format (DXT5, RGBA8888, etc.)
        // TODO: Indent these better (I'm lazy)
        GimpChoice *choice_image_format = gimp_choice_new_with_values(
            "RGBA8888",                     (int)vtfpp::ImageFormat::RGBA8888, "RGBA8888", NULL,
            "ABGR8888",                     (int)vtfpp::ImageFormat::ABGR8888, "ABGR8888", NULL,
            "RGB888",                       (int)vtfpp::ImageFormat::RGB888, "RGB888", NULL,
            "BGR888",                       (int)vtfpp::ImageFormat::BGR888, "BGR888", NULL,
            "RGB565",                       (int)vtfpp::ImageFormat::RGB565, "RGB565", NULL,
            "I8",                           (int)vtfpp::ImageFormat::I8, "I8", NULL,
            "IA88",                         (int)vtfpp::ImageFormat::IA88, "IA88", NULL,
            "P8",                           (int)vtfpp::ImageFormat::P8, "P8", NULL,
            "A8",                           (int)vtfpp::ImageFormat::A8, "A8", NULL,
            "RGB888_BLUESCREEN",            (int)vtfpp::ImageFormat::RGB888_BLUESCREEN, "RGB888_BLUESCREEN", NULL,
            "BGR888_BLUESCREEN",            (int)vtfpp::ImageFormat::BGR888_BLUESCREEN, "BGR888_BLUESCREEN", NULL,
            "ARGB8888",                     (int)vtfpp::ImageFormat::ARGB8888, "ARGB8888", NULL,
            "BGRA8888",                     (int)vtfpp::ImageFormat::BGRA8888, "BGRA8888", NULL,
            "DXT1",                         (int)vtfpp::ImageFormat::DXT1, "DXT1", NULL,
            "DXT3",                         (int)vtfpp::ImageFormat::DXT3, "DXT3", NULL,
            "DXT5",                         (int)vtfpp::ImageFormat::DXT5, "DXT5", NULL,
            "BGRX8888",                     (int)vtfpp::ImageFormat::BGRX8888, "BGRX8888", NULL,
            "BGR565",                       (int)vtfpp::ImageFormat::BGR565, "BGR565", NULL,
            "BGRX5551",                     (int)vtfpp::ImageFormat::BGRX5551, "BGRX5551", NULL,
            "BGRA4444",                     (int)vtfpp::ImageFormat::BGRA4444, "BGRA4444", NULL,
            "DXT1_ONE_BIT_ALPHA",           (int)vtfpp::ImageFormat::DXT1_ONE_BIT_ALPHA, "DXT1_ONE_BIT_ALPHA", NULL,
            "BGRA5551",                     (int)vtfpp::ImageFormat::BGRA5551, "BGRA5551", NULL,
            "UV88",                         (int)vtfpp::ImageFormat::UV88, "UV88", NULL,
            "UVWQ8888",                     (int)vtfpp::ImageFormat::UVWQ8888, "UVWQ8888", NULL,
            "RGBA16161616F",                (int)vtfpp::ImageFormat::RGBA16161616F, "RGBA16161616F", NULL,
            "RGBA16161616",                 (int)vtfpp::ImageFormat::RGBA16161616, "RGBA16161616", NULL,
            "UVLX8888",                     (int)vtfpp::ImageFormat::UVLX8888, "UVLX8888", NULL,
            "R32F",                         (int)vtfpp::ImageFormat::R32F, "R32F", NULL,
            "RGB323232F",                   (int)vtfpp::ImageFormat::RGB323232F, "RGB323232F", NULL,
            "RGBA32323232F",                (int)vtfpp::ImageFormat::RGBA32323232F, "RGBA32323232F", NULL,

            "RG1616F",                      (int)vtfpp::ImageFormat::RG1616F, "RG1616F", NULL,
            "RG3232F",                      (int)vtfpp::ImageFormat::RG3232F, "RG3232F", NULL,
            "RGBX8888",                     (int)vtfpp::ImageFormat::RGBX8888, "RGBX8888", NULL,
            "EMPTY",                        (int)vtfpp::ImageFormat::EMPTY, "EMPTY", NULL,
            "ATI2N",                        (int)vtfpp::ImageFormat::ATI2N, "ATI2N", NULL,
            "ATI1N",                        (int)vtfpp::ImageFormat::ATI1N, "ATI1N", NULL,
            "RGBA1010102",                  (int)vtfpp::ImageFormat::RGBA1010102, "RGBA1010102", NULL,
            "BGRA1010102",                  (int)vtfpp::ImageFormat::BGRA1010102, "BGRA1010102", NULL,
            "R16F",                         (int)vtfpp::ImageFormat::R16F, "R16F", NULL,

            "CONSOLE_BGRX8888_LINEAR",      (int)vtfpp::ImageFormat::CONSOLE_BGRX8888_LINEAR, "CONSOLE_BGRX8888_LINEAR", NULL,
            "CONSOLE_RGBA8888_LINEAR",      (int)vtfpp::ImageFormat::CONSOLE_RGBA8888_LINEAR, "CONSOLE_RGBA8888_LINEAR", NULL,
            "CONSOLE_ABGR8888_LINEAR",      (int)vtfpp::ImageFormat::CONSOLE_ABGR8888_LINEAR, "CONSOLE_ABGR8888_LINEAR", NULL,
            "CONSOLE_ARGB8888_LINEAR",      (int)vtfpp::ImageFormat::CONSOLE_ARGB8888_LINEAR, "CONSOLE_ARGB8888_LINEAR", NULL,
            "CONSOLE_BGRA8888_LINEAR",      (int)vtfpp::ImageFormat::CONSOLE_BGRA8888_LINEAR, "CONSOLE_BGRA8888_LINEAR", NULL,
            "CONSOLE_RGB888_LINEAR",        (int)vtfpp::ImageFormat::CONSOLE_RGB888_LINEAR, "CONSOLE_RGB888_LINEAR", NULL,
            "CONSOLE_BGR888_LINEAR",        (int)vtfpp::ImageFormat::CONSOLE_BGR888_LINEAR, "CONSOLE_BGR888_LINEAR", NULL,
            "CONSOLE_BGRX5551_LINEAR",      (int)vtfpp::ImageFormat::CONSOLE_BGRX5551_LINEAR, "CONSOLE_BGRX5551_LINEAR", NULL,
            "CONSOLE_I8_LINEAR",            (int)vtfpp::ImageFormat::CONSOLE_I8_LINEAR, "CONSOLE_I8_LINEAR", NULL,
            "CONSOLE_RGBA16161616_LINEAR",  (int)vtfpp::ImageFormat::CONSOLE_RGBA16161616_LINEAR, "CONSOLE_RGBA16161616_LINEAR", NULL,
            "CONSOLE_BGRX8888_LE",          (int)vtfpp::ImageFormat::CONSOLE_BGRX8888_LE, "CONSOLE_BGRX8888_LE", NULL,
            "CONSOLE_BGRA8888_LE",          (int)vtfpp::ImageFormat::CONSOLE_BGRA8888_LE, "CONSOLE_BGRA8888_LE", NULL,
        
            "R8",                           (int)vtfpp::ImageFormat::R8, "R8", NULL,
            "BC7",                          (int)vtfpp::ImageFormat::BC7, "BC7", NULL,
            "BC6H",                         (int)vtfpp::ImageFormat::BC6H, "BC6H", NULL,
            NULL
        );
        gimp_procedure_add_choice_argument(
            procedure,
            "image_format",
            "Image format",
            "Image format to use."
            "\nRecommended: DXT1 for regular textures without alpha, DXT5 for textures with alpha."
            "\nIf you're developing specifically for an engine based on Strata Source, then use BC7.",
            choice_image_format,
            // TODO: Change this selection based on whether or not the current image has alpha?
            "DXT1",
            G_PARAM_READWRITE
        );

        // Type (Standard, Environment Map, Volumetric Texture)
        GimpChoice *choice_image_type = gimp_choice_new_with_values(
            "standard",     0, "Standard", NULL,
            "envmap",       1, "Environment Map", NULL,
            "volumetric",   2, "Volumetric Texture", NULL,
            NULL
        );
        gimp_procedure_add_choice_argument(
            procedure,
            "image_type",
            "Image type",
            "Image type (Standard, Environment Map, or Volumetric Texture)."
            "\nRecommended: Standard, unless you're making skyboxes, then use Environment Map.",
            choice_image_type,
            "standard",
            G_PARAM_READWRITE
        );

        // Mipmaps (as well as an option of whether or not to even generate them)
        GimpChoice *choice_mipmaps = gimp_choice_new_with_values(
            "none",         -1,                                                         "None (don't generate mipmaps)", NULL,
            "default",      (int)vtfpp::ImageConversion::ResizeFilter::DEFAULT,         "Default", NULL,
            "box",          (int)vtfpp::ImageConversion::ResizeFilter::BOX,             "Box", NULL,
            "bilinear",     (int)vtfpp::ImageConversion::ResizeFilter::BILINEAR,        "Bilinear", NULL,
            "cubic",        (int)vtfpp::ImageConversion::ResizeFilter::CUBIC_BSPLINE,   "Cubic", NULL,
            "catmull",      (int)vtfpp::ImageConversion::ResizeFilter::CATMULL_ROM,     "Catmull/Catrom", NULL,
            "mitchell",     (int)vtfpp::ImageConversion::ResizeFilter::MITCHELL,        "Mitchell", NULL,
            "point",        (int)vtfpp::ImageConversion::ResizeFilter::POINT_SAMPLE,    "Point", NULL,
            "kaiser",       (int)vtfpp::ImageConversion::ResizeFilter::KAISER,          "Kaiser", NULL,
            NULL
        );
        gimp_procedure_add_choice_argument(
            procedure,
            "mipmap_filter",
            "Mipmap filter",
            "Mipmap resize filter to use."
            "\nRecommended: Kaiser.",
            choice_mipmaps,
            "kaiser",
            G_PARAM_READWRITE
        );

        // Resize method (how to resize the image when the width and height aren't a power-of-two)
        GimpChoice *choice_resize_method = gimp_choice_new_with_values(
            "bigger",   (int)vtfpp::ImageConversion::ResizeMethod::POWER_OF_TWO_BIGGER,     "Power of two (bigger)", NULL,
            "smaller",  (int)vtfpp::ImageConversion::ResizeMethod::POWER_OF_TWO_SMALLER,    "Power of two (smaller)", NULL,
            "nearest",  (int)vtfpp::ImageConversion::ResizeMethod::POWER_OF_TWO_NEAREST,    "Power of two (nearest)", NULL,
            NULL
        );
        gimp_procedure_add_choice_argument(
            procedure,
            "resize_method",
            "Resize method",
            "Resize method to use when the image isn't a power-of-two in either its width or height."
            "\nBigger: Always round up to the nearest power of two."
            "\nSmaller: Always round down to the nearest power of two."
            "\nNearest: Round to whichever power of two is closer.",
            choice_resize_method,
            "bigger",
            G_PARAM_READWRITE
        );

        gimp_procedure_add_boolean_argument(
            procedure,
            "thumbnail_enabled",
            "Write thumbnail",
            "If enabled, write thumbnail to VTF."
            "\nThis should almost always be enabled.",
            TRUE,
            G_PARAM_READWRITE
        );

        // TODO: implement
        gimp_procedure_add_boolean_argument(
            procedure,
            "merge_layers_enabled",
            "Merge layers",
            "If enabled, all GIMP layers will be merged into a single image in the VTF."
            "\nKeep this disabled if you need to have multiple frames or faces in your VTF.",
            FALSE,
            G_PARAM_READWRITE
        );

        gimp_procedure_add_boolean_argument(
            procedure,
            "recompute_reflectivity_enabled",
            "Recompute reflectivity",
            "If enabled, the reflectivity of the VTF will be recomputed."
            "\nYou should probably keep this enabled unless you know what you're doing.",
            TRUE,
            G_PARAM_READWRITE
        );

        gimp_procedure_add_double_argument(
            procedure,
            "bumpmap_scale",
            "Bumpmap scale",
            "Bumpmap scale",
            0.0f,
            10.0f,
            1.0f,
            G_PARAM_READWRITE
        );

        gimp_export_procedure_set_support_exif(GIMP_EXPORT_PROCEDURE(procedure), false);
        gimp_export_procedure_set_support_iptc(GIMP_EXPORT_PROCEDURE(procedure), false);
        gimp_export_procedure_set_support_xmp(GIMP_EXPORT_PROCEDURE(procedure), false);
        gimp_export_procedure_set_support_profile(GIMP_EXPORT_PROCEDURE(procedure), false);
        gimp_export_procedure_set_support_thumbnail(GIMP_EXPORT_PROCEDURE(procedure), false);
        gimp_export_procedure_set_support_comment(GIMP_EXPORT_PROCEDURE(procedure), false);
    }

    return procedure;
}

// * Useful reference:
// - https://gitlab.gnome.org/GNOME/gimp/-/blob/master/plug-ins/common/file-png.c
// - https://gitlab.gnome.org/GNOME/gimp/-/blob/master/plug-ins/file-jpeg/jpeg-load.c
// - https://fossies.org/diffs/gimp/2.10.38_vs_3.0.0/libgimp/gimppixelrgn.h-diff.html
static GimpValueArray *gimp_vtf_load(
    GimpProcedure *procedure,
    GimpRunMode run_mode,
    GFile *file,
    GimpMetadata *metadata,
    GimpMetadataLoadFlags *flags,
    GimpProcedureConfig *config,
    gpointer run_data
) {
    GimpValueArray *return_vals;
    GError *error = NULL;

    // Attempt to parse the VTF file
    GimpImage *image = load_image(file, &error);
    // Generic catch-all if the image wasn't loaded for whatever reason
    if (!image) {
        return gimp_procedure_new_return_values(procedure, GIMP_PDB_EXECUTION_ERROR, error);
    }

    return_vals = gimp_procedure_new_return_values(procedure, GIMP_PDB_SUCCESS, NULL);

    GIMP_VALUES_SET_IMAGE(return_vals, 1, image);

    return return_vals;
}

// Gets a GFile, returns a GimpImage.
// Most of the VTF loading work is done here.
static GimpImage *load_image(GFile *file, GError **error) {
    char *file_path = g_file_get_path(file);

    // TODO: error handling here
    vtfpp::VTF vtf_file = vtfpp::VTF(file_path, false);
    int width = vtf_file.getWidth();
    int height = vtf_file.getHeight();

    // TODO: GimpImageBaseType can be GIMP_RGB, GIMP_GRAY or GIMP_INDEXED.
    //  VTF has grayscale formats, not sure if it has indexed ones.
    //  Will have to change type based on the file format detected.

    GimpImage *image = gimp_image_new_with_precision(
        width,
        height,
        GIMP_RGB,
        GIMP_PRECISION_U8_NON_LINEAR
    );

    // For each frame, for each face
    // https://developer.valvesoftware.com/wiki/VTF_(Valve_Texture_Format)#Image_data_formats
    int frame_count = vtf_file.getFrameCount();
    int face_count = vtf_file.getFaceCount();
    int layer_number = 0;
    for (int fr_i = 0; fr_i < frame_count; fr_i++) {
        for (int fa_i = 0; fa_i < face_count; fa_i++) {
            gchar *layer_name = g_strdup_printf("Layer %.3d", layer_number + 1);
            layer_number++;
            
            // TODO: same as before, but for GimpImageType
            //  We'll just use GIMP_RGBA_IMAGE for now (RGB with alpha)
            GimpLayer *layer = gimp_layer_new(
                image,
                layer_name,
                width,
                height,
                GIMP_RGBA_IMAGE,
                100,
                gimp_image_get_default_new_layer_mode(image)
            );
            gimp_image_insert_layer(image, layer, NULL, 0);
            g_free(layer_name);

            GeglBuffer *buffer = gimp_drawable_get_buffer(GIMP_DRAWABLE(layer));
            std::vector<std::byte> image_data_rgba = vtf_file.getImageDataAsRGBA8888(0, fr_i, fa_i, 0);

            // Get the bits per pixel for the RGBA8888 format (the one we're using above)
            // Divide by 8 to get bytes
            int bpp = vtfpp::ImageFormatDetails::bpp(vtfpp::ImageFormat::RGBA8888) / 8;
            uint8_t *dst_buf = g_new(uint8_t, width * height * bpp);
            for (int i = 0; i < image_data_rgba.size(); i++) {
                dst_buf[i] = (uint8_t)image_data_rgba[i];
            }

            gegl_buffer_set(
                buffer,
                GEGL_RECTANGLE(0, 0, width, height),
                0,
                babl_format_with_space(
                    "R'G'B'A u8",
                    gimp_drawable_get_format(GIMP_DRAWABLE(layer))
                ),
                dst_buf,
                GEGL_AUTO_ROWSTRIDE
            );

            g_object_unref(buffer);
        }
    }

    return image;
}

static GimpValueArray *gimp_vtf_export(
    GimpProcedure *procedure,
    GimpRunMode run_mode,
    GimpImage *image,
    GFile *file,
    GimpExportOptions *options,
    GimpMetadata *metadata,
    GimpProcedureConfig *config,
    gpointer run_data
) {
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;
    GimpExportReturn export_type = GIMP_EXPORT_IGNORE;
    GList *drawables;
    GimpImage *orig_image;
    gboolean has_alpha;
    GError *error = NULL;

    gegl_init(NULL, NULL);

    orig_image = image;

    export_type = gimp_export_options_get_image(options, &image);
    // We have to reverse the drawables list when exporting,
    //  as GIMP sorts it top to bottom by default
    drawables = g_list_reverse(gimp_image_list_layers(image));
    has_alpha = gimp_drawable_has_alpha(GIMP_DRAWABLE(drawables->data));

    // https://gitlab.gnome.org/GNOME/gimp/-/blob/master/plug-ins/file-jpeg/jpeg.c#L448
    switch (run_mode) {
        case GIMP_RUN_NONINTERACTIVE:
            g_object_set(config, "show-preview", FALSE, NULL);
            break;

        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
            {
                gimp_ui_init(PROC_VTF_BINARY);

                if (!export_dialog(orig_image, procedure, config)) {
                    status = GIMP_PDB_CANCEL;
                }
            }
            break;
    }

    // If we're ready to continue with exporting the image to disk
    if (status == GIMP_PDB_SUCCESS) {
        gboolean export_successful = export_image(
            file,
            image,
            drawables,
            orig_image,
            config,
            has_alpha,
            &error
        );

        if (!export_successful) {
            status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

    if (export_type == GIMP_EXPORT_EXPORT) {
        gimp_image_delete(image);
    }

    g_list_free(drawables);

    return gimp_procedure_new_return_values(procedure, status, NULL);
}

static gboolean export_dialog(
    GimpImage *image,
    GimpProcedure *procedure,
    GimpProcedureConfig *config
) {
    GtkWidget *dialog = gimp_export_procedure_dialog_new(
        GIMP_EXPORT_PROCEDURE(procedure),
        config,
        image
    );

    gimp_procedure_dialog_fill(
        GIMP_PROCEDURE_DIALOG(dialog),
        "image_type",
        "version",
        "image_format",
        "mipmap_filter",
        "resize_method",
        "bumpmap_scale",
        "thumbnail_enabled",
        "recompute_reflectivity_enabled",
        "merge_layers_enabled",
        NULL
    );
    
    gboolean run_successful = gimp_procedure_dialog_run(GIMP_PROCEDURE_DIALOG(dialog));

    gtk_widget_destroy(dialog);
    
    return run_successful;
}

static gboolean export_image(GFile *file,
    GimpImage *image,
    GList *drawables,
    GimpImage *orig_image,
    GimpProcedureConfig *config,
    gboolean has_alpha,
    GError **error
) {
    // This is specifically the VTF minor version. So if the user chose 7.4, this would be '4'
    int file_version;
    // Image format (DXT1, RGBA8888, etc.)
    vtfpp::ImageFormat image_format;
    // TODO (image types):
    //  - If standard, do nothing special.
    //  - If environment map, set related flag, and use CreationOptions.isCubeMap
    //  - If volumetric texture, set related flag
    VTFImageType image_type;
    // Mipmap filter. '-1' is a special value and means "don't generate mipmaps at all"
    int mipmap_filter;
    // Resize method (power-of-two bigger, smaller, or nearest)
    vtfpp::ImageConversion::ResizeMethod resize_method;
    bool thumbnail_enabled;
    // TODO: implement
    bool merge_layers_enabled;
    bool recompute_reflectivity_enabled;
    double bumpmap_scale;

    file_version = gimp_procedure_config_get_choice_id(config, "version");
    image_type = (VTFImageType)gimp_procedure_config_get_choice_id(config, "image_type");
    mipmap_filter = gimp_procedure_config_get_choice_id(config, "mipmap_filter");
    image_format = (vtfpp::ImageFormat)gimp_procedure_config_get_choice_id(config, "image_format");
    resize_method = (vtfpp::ImageConversion::ResizeMethod)gimp_procedure_config_get_choice_id(config, "resize_method");
    g_object_get(
        config,
        "thumbnail_enabled",                &thumbnail_enabled,
        "merge_layers_enabled",             &merge_layers_enabled,
        "recompute_reflectivity_enabled",   &recompute_reflectivity_enabled,
        "bumpmap_scale",                    &bumpmap_scale,
        NULL
    );

    bool should_compute_mips = (mipmap_filter == -1) ? false : true;

    // Get width and height of the GIMP image
    GimpDrawable *drawable_reference = GIMP_DRAWABLE(drawables->data);
    GeglBuffer *buffer_for_res = gimp_drawable_get_buffer(drawable_reference);
    int width = gegl_buffer_get_width(buffer_for_res);
    int height = gegl_buffer_get_height(buffer_for_res);
    g_object_unref(buffer_for_res);

    // Set up some basic information in the exported VTF
    vtfpp::VTF export_vtf;
    export_vtf.setVersion(7, file_version);
    export_vtf.addFlags(vtfpp::VTF::FLAG_SRGB);
    export_vtf.setImageResizeMethods(resize_method, resize_method);
    export_vtf.setSize(width, height, vtfpp::ImageConversion::ResizeFilter::DEFAULT);

    // Set images inside the VTF
    // TODO: export multiple layers as multiple frames (& equivalent for faces)
    int layer_count = g_list_length(drawables);

    // Depending on whether the image is a standard image or envmap/volumetric,
    //  write the images either as frames or as faces
    if (image_type == VTFImageType::TYPE_STANDARD) {
        export_vtf.setFrameCount(layer_count);
    } else {
        export_vtf.setFaceCount(true, layer_count >= 7);
    }

    for (int layer_index = 0; layer_index < layer_count; layer_index++) {
        GList *layer_at_nth = g_list_nth(drawables, layer_index);
        GimpDrawable *drawable_for_this_layer = GIMP_DRAWABLE(layer_at_nth->data);
        GeglBuffer *buffer_for_this_layer = gimp_drawable_get_buffer(drawable_for_this_layer);

        int bpp = vtfpp::ImageFormatDetails::bpp(vtfpp::ImageFormat::RGBA8888) / 8;
        int file_bytes_count = width * height * bpp;
        // Take bytes from the GIMP drawable buffer and put it in this vector
        uint8_t *raw_bytes = g_new(uint8_t, file_bytes_count);
        gegl_buffer_get(
            buffer_for_this_layer,
            GEGL_RECTANGLE(0, 0, width, height),
            1.0,
            gimp_drawable_get_format(drawable_for_this_layer),
            raw_bytes,
            GEGL_AUTO_ROWSTRIDE,
            GEGL_ABYSS_NONE
        );
        g_object_unref(buffer_for_this_layer);

        // The vtfpp library can't seem to interface with uint8_t directly, so we have to
        //  move data to a vector
        std::vector<std::byte> raw_bytes_vec;
        for (int i = 0; i < file_bytes_count; i++) {
            raw_bytes_vec.push_back((std::byte)raw_bytes[i]);
        }

        // Depending on whether the image is a standard image or envmap/volumetric,
        //  write the images either as frames or as faces
        uint16_t frame_index = 0;
        uint8_t face_index = 0;
        if (image_type == VTFImageType::TYPE_STANDARD) {
            frame_index = layer_index;
        } else {
            face_index = layer_index;
        }

        // Take the bytes from the vector and parse it as a VTF image layer
        bool bytes_to_image_successful = export_vtf.setImage(
            raw_bytes_vec,
            // Because the raw_bytes_vec is stored using 4 bytes per pixel,
            //  we *must* use RGBA8888 when we initially import from the GIMP layers to the VTF.
            // However, the user's selected VTF format will still be respected once we write to disk.
            vtfpp::ImageFormat::RGBA8888,
            width,
            height,
            // This is specifically the resize method used when the user gives the image in GIMP
            //  an invalid size. It is *not* used when generating mipmaps (as far as I'm aware).
            // Might make this configurable to the user, but there is an argument to be made that
            //  if the user wanted to resize the image, they could just do it in GIMP. So for now,
            //  I won't add it.
            vtfpp::ImageConversion::ResizeFilter::DEFAULT,
            0,
            frame_index,
            face_index,
            0
        );

        if (!bytes_to_image_successful) {
            g_warning("Could not successfully call vtf.setImage() for layer %d", layer_index);
            //return false;
        }
    }

    //
    // Compute VTF settings
    //
    // TODO: set flags here

    // TODO: set start frame here

    export_vtf.setBumpMapScale(bumpmap_scale);

    if (should_compute_mips) {
        export_vtf.setMipCount(vtfpp::ImageDimensions::getRecommendedMipCountForDims(image_format, width, height));
        export_vtf.computeMips((vtfpp::ImageConversion::ResizeFilter)mipmap_filter);
    } else {
        export_vtf.setMipCount(1);
    }

    if (thumbnail_enabled) {
        export_vtf.computeThumbnail(vtfpp::ImageConversion::ResizeFilter::DEFAULT);
    } else {
        export_vtf.removeThumbnail();
    }

    if (recompute_reflectivity_enabled) {
        export_vtf.computeReflectivity();
    }

    export_vtf.computeTransparencyFlags();

    export_vtf.setFormat(image_format, vtfpp::ImageConversion::ResizeFilter::DEFAULT);

    // TODO: set compression method here
    
    // TODO: set compression level here

    // Write VTF to file on disk
    bool export_successful = export_vtf.bake(g_file_get_path(file));

    return export_successful;
}

GIMP_MAIN(GIMP_VTF_TYPE);
