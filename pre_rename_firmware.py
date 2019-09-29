Import("env")

buildFlags = env.ParseFlags(env['BUILD_FLAGS'])
buildDefines = {k: v for (k, v) in buildFlags.get("CPPDEFINES")}

env.Replace(PROGNAME="pidflightlap_%s_%s" % (buildDefines.get("VERSION"), buildDefines.get("VARIANT")))