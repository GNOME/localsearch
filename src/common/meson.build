tracker_miners_common_sources = [
  'tracker-dbus.c',
  'tracker-debug.c',
  'tracker-file-utils.c',
  'tracker-ioprio.c',
  'tracker-miner.c',
  'tracker-extract-rules-manager.c',
  'tracker-sched.c',
  'tracker-systemd.c',
  'tracker-term-utils.c',
  'tracker-type-utils.c',
  'tracker-utils.c',
  'tracker-locale.c',
  'tracker-seccomp.c',
]

if have_landlock
  tracker_miners_common_sources += 'tracker-landlock.c'
endif

tracker_miners_common_dependencies = [glib, gio, gio_unix, libmath, tracker_sparql]

if battery_detection_library_name == 'upower'
  tracker_miners_common_sources += 'tracker-power-upower.c'
  tracker_miners_common_dependencies += upower
endif

tracker_miners_common_private_dependencies = [charset_library]
if libseccomp.found()
  tracker_miners_common_private_dependencies += [libseccomp]
endif

if build_machine.system() == 'openbsd'
  libkvm = meson.get_compiler('c').find_library('kvm')
  tracker_miners_common_dependencies += libkvm
endif

libtracker_miners_common = static_library('tracker-miners-common',
  tracker_miners_common_sources,
  dependencies: tracker_miners_common_dependencies + tracker_miners_common_private_dependencies,
  c_args: tracker_c_args + [
    '-DTRACKERSHAREDIR="@0@"'.format(tracker_versioned_name),
    '-DLIBDIR="@0@"'.format(get_option ('libdir')),
    '-DDATADIR="@0@"'.format(get_option ('datadir')),
    '-DPREFIX="@0@"'.format(get_option ('prefix')),
    '-DBUILDROOT="@0@"'.format(meson.global_build_root()),
    '-DSRCROOT="@0@"'.format(meson.global_source_root()),
    '-DLIBEXECDIR="@0@"'.format(join_paths(get_option('prefix'), get_option('libexecdir'))),
    '-DTRACKER_EXTRACTOR_RULES_DIR="@0@"'.format(tracker_extract_rules_dir),
    '-DTRACKER_EXTRACTORS_DIR="@0@"'.format(tracker_extract_modules_dir)
  ],
  include_directories: [configinc, srcinc],
)

commoninc = include_directories('.')

tracker_miners_common_dep = declare_dependency(
  link_with: libtracker_miners_common,
  dependencies: tracker_miners_common_dependencies,
  include_directories: [configinc, srcinc, commoninc],
)
