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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

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
