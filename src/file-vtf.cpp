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

#include <babl/babl.h>
#include <cstddef>
#include <gegl-buffer.h>
#include <gegl-types.h>
#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpbatchprocedure.h>
#include <libgimp/gimpfileprocedure.h>
#include <libgimp/gimpimageprocedure.h>
#include <libgimp/gimptypes.h>
#include <libgimpbase/gimpbaseenums.h>
#include <libgimpbase/gimpmetadata.h>
#include <vector>
#include "vtfpp/VTF.h"

// Procedures prefixed with 'plug-in-chev' to avoid procedure name conflicts, like another VTF loading plugin
#define PROC_VTF_LOAD "plug-in-chev-file-vtf-load"
#define PROC_VTF_EXPORT "plug-in-chev-file-vtf-export"

struct _GimpVtf {
    GimpPlugIn parent_instance;
};

#define GIMP_VTF_TYPE (gimp_vtf_get_type())
G_DECLARE_FINAL_TYPE(GimpVtf, gimp_vtf, GIMP_VTF,, GimpPlugIn)
G_DEFINE_TYPE(GimpVtf, gimp_vtf, GIMP_TYPE_PLUG_IN)

// Headers
static GList *gimp_vtf_query_procedures(GimpPlugIn *plugin);
static GimpProcedure *gimp_vtf_create_procedure(GimpPlugIn *plugin, const gchar *name);
static GimpValueArray *gimp_vtf_load(GimpProcedure *procedure,
    GimpRunMode run_mode,
    GFile *file,
    GimpMetadata *metadata,
    GimpMetadataLoadFlags *flags,
    GimpProcedureConfig *config,
    gpointer run_data);
static GimpImage *load_image(GFile *file, GError **error);
static GimpValueArray *gimp_vtf_export(GimpProcedure *procedure,
    GimpRunMode run_mode,
    GimpImage *image,
    GimpDrawable **drawables,
    GimpProcedureConfig *config,
    gpointer run_data);

static void gimp_vtf_class_init(GimpVtfClass *gclass) {
    GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS(gclass);

    // Called after every update of either GIMP or the plugin
    plug_in_class->query_procedures = gimp_vtf_query_procedures;

    // Returns interface of this procedure + metadata like title, menu path, author
    plug_in_class->create_procedure = gimp_vtf_create_procedure;
}

static void gimp_vtf_init(GimpVtf *gimp_vtf) {}

static GList *gimp_vtf_query_procedures(GimpPlugIn *plugin) {
    GList *list = g_list_append(NULL, g_strdup(PROC_VTF_LOAD));
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
        gimp_procedure_set_menu_label(procedure, "_VTF image");
        gimp_procedure_set_documentation(
            procedure,
            "Loads files in VTF file format",
            "This plug-in loads Valve Texture Format (VTF) files.",
            NULL
        );
        gimp_procedure_set_attribution(
            procedure,
            "Chev <riskyrains@proton.me>",
            "GPL-3.0",
            "2025"
        );
        gimp_file_procedure_set_mime_types(GIMP_FILE_PROCEDURE(procedure), "image/vtf");
        gimp_file_procedure_set_extensions(GIMP_FILE_PROCEDURE(procedure), "vtf");
        gimp_file_procedure_set_magics(GIMP_FILE_PROCEDURE(procedure), "0,string,VTF\000");
    } else if (g_strcmp0(name, PROC_VTF_EXPORT) == 0) {
        procedure = gimp_image_procedure_new(
            plugin, name, GIMP_PDB_PROC_TYPE_PLUGIN, gimp_vtf_export, NULL, NULL);
    }

    return procedure;
}

// * Useful reference:
// - https://gitlab.gnome.org/GNOME/gimp/-/blob/master/plug-ins/common/file-png.c
// - https://gitlab.gnome.org/GNOME/gimp/-/blob/master/plug-ins/file-jpeg/jpeg-load.c
// - https://fossies.org/diffs/gimp/2.10.38_vs_3.0.0/libgimp/gimppixelrgn.h-diff.html
static GimpValueArray *gimp_vtf_load(GimpProcedure *procedure,
                                    GimpRunMode run_mode,
                                    GFile *file,
                                    GimpMetadata *metadata,
                                    GimpMetadataLoadFlags *flags,
                                    GimpProcedureConfig *config,
                                    gpointer run_data) {
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

            // TODO: something goes wrong with color calculation here
            //  the image looked desaturated/washed out

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

static GimpValueArray *gimp_vtf_export(GimpProcedure *procedure,
                                       GimpRunMode run_mode,
                                       GimpImage *image,
                                       GimpDrawable **drawables,
                                       GimpProcedureConfig *config,
                                       gpointer run_data) {
    gimp_message("Hello World!!");

    return gimp_procedure_new_return_values(procedure, GIMP_PDB_SUCCESS, NULL);
}

GIMP_MAIN(GIMP_VTF_TYPE);
