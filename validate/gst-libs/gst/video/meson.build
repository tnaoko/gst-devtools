cairo_dep = dependency('cairo', required: false)
if cairo_dep.found()
    video = static_library(
        'gstvalidatevideo',
        'gstvalidatessim.c', 'gssim.c',
        include_directories : inc_dirs,
        dependencies : [gst_dep, gst_video_dep, gst_pbutils_dep, glib_dep, cairo_dep, gio_dep,
            mathlib],
    )
endif
