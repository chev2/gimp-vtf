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

// Headers
static GList *gimp_vtf_query_procedures(
    GimpPlugIn *plugin
);
static GimpProcedure *gimp_vtf_create_procedure(
    GimpPlugIn *plugin, const gchar *name
);
static GimpValueArray *gimp_vtf_load(
    GimpProcedure *procedure,
    GimpRunMode run_mode,
    GFile *file,
    GimpMetadata *metadata,
    GimpMetadataLoadFlags *flags,
    GimpProcedureConfig *config,
    gpointer run_data);
static GimpImage *load_image(
    GFile *file,
    GError **error
);
static GimpValueArray *gimp_vtf_export(
    GimpProcedure *procedure,
    GimpRunMode run_mode,
    GimpImage *image,
    GFile *file,
    GimpExportOptions *options,
    GimpMetadata *metadata,
    GimpProcedureConfig *config,
    gpointer run_data
);
static gboolean export_dialog(
    GimpImage *image,
    GimpProcedure *procedure,
    GimpProcedureConfig *config
);
static gboolean export_image(
    GFile *file,
    GimpImage *image,
    GimpDrawable *drawable,
    GimpImage *orig_image,
    GimpProcedureConfig *config,
    gboolean has_alpha,
    GError **error
);

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
        gimp_file_procedure_set_mime_types(GIMP_FILE_PROCEDURE(procedure), "image/vtf");
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
        gimp_file_procedure_set_mime_types(GIMP_FILE_PROCEDURE(procedure), "image/vtf");
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
            "VTF file version (7.0 to 7.6).\nRecommended: Use 7.4 for best compatibility.",
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
            "Image format to use.\nRecommended: DXT1 for regular textures without alpha, DXT5 for textures with alpha.",
            choice_image_format,
            // TODO: Change this selection based on whether or not the current image has alpha
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
            "Image type (Standard, Environment Map, or Volumetric Texture).\nRecommended: Standard, unless you're making skyboxes, then use Environment Map.",
            choice_image_type,
            "standard",
            G_PARAM_READWRITE
        );

        gimp_export_procedure_set_support_exif(GIMP_EXPORT_PROCEDURE(procedure), false);
        gimp_export_procedure_set_support_iptc(GIMP_EXPORT_PROCEDURE(procedure), false);
        gimp_export_procedure_set_support_xmp(GIMP_EXPORT_PROCEDURE(procedure), false);
        gimp_export_procedure_set_support_profile(GIMP_EXPORT_PROCEDURE(procedure), false);
        gimp_export_procedure_set_support_thumbnail(GIMP_EXPORT_PROCEDURE(procedure), true);
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
    char *file_path = g_file_get_path(file);
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
    for (int fr_i = 0; fr_i < frame_count; fr_i++) {
        for (int fa_i = 0; fa_i < face_count; fa_i++) {
            gchar *layer_name = g_strdup_printf("Frame %d", fa_i + 1);
            
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

            // 4 bytes per pixel (r, g, b, a)
            uint8_t *dst_buf = g_new(uint8_t, width * height * 4);
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
    drawables = gimp_image_list_layers(image);
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
            GIMP_DRAWABLE(drawables->data),
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

    gimp_message("Ran successfully");

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
        "image_type", "version", "image_format",
        NULL
    );
    
    gboolean run_successful = gimp_procedure_dialog_run(GIMP_PROCEDURE_DIALOG(dialog));

    gtk_widget_destroy(dialog);
    
    return run_successful;
}

static gboolean export_image(GFile *file,
    GimpImage *image,
    GimpDrawable *drawable,
    GimpImage *orig_image,
    GimpProcedureConfig *config,
    gboolean has_alpha,
    GError **error
) {
    const Babl *file_format = gimp_drawable_get_format(drawable);

    int file_version = gimp_procedure_config_get_choice_id(config, "version");
    int image_format = gimp_procedure_config_get_choice_id(config, "image_format");
    int image_type = gimp_procedure_config_get_choice_id(config, "image_type");

    GeglBuffer *buffer = gimp_drawable_get_buffer(drawable);
    int width = gegl_buffer_get_width(buffer);
    int height = gegl_buffer_get_height(buffer);

    vtfpp::VTF::CreationOptions creation_options;
    creation_options.minorVersion = file_version;
    creation_options.outputFormat = (vtfpp::ImageFormat)image_format;

    // Create a new VTF with the user's selected options
    vtfpp::VTF export_vtf = vtfpp::VTF::create(
        (vtfpp::ImageFormat)image_format,
        width,
        height,
        creation_options
    );

    // Take bytes from the GIMP drawable buffer and put it in this vector
    int file_bytes_count = width * height * 4;
    uint8_t *raw_bytes = g_new(uint8_t, file_bytes_count);
    gegl_buffer_get(
        buffer,
        GEGL_RECTANGLE(0, 0, width, height),
        1.0,
        file_format,
        raw_bytes,
        GEGL_AUTO_ROWSTRIDE,
        GEGL_ABYSS_NONE
    );
    g_object_unref(buffer);

    std::vector<std::byte> raw_bytes_vec;
    for (int i = 0; i < file_bytes_count; i++) {
        raw_bytes_vec.push_back((std::byte)raw_bytes[i]);
    }

    // Take the bytes from the vector and parse it as a VTF image layer
    bool bytes_to_image_successful = export_vtf.setImage(
        raw_bytes_vec,
        vtfpp::ImageFormat::RGBA8888,
        width,
        height,
        vtfpp::ImageConversion::ResizeFilter::DEFAULT,
        0,
        0,
        0,
        0
    );

    // Write VTF to file
    bool export_successful = export_vtf.bake(g_file_get_path(file));

    return export_successful;
}

GIMP_MAIN(GIMP_VTF_TYPE);
