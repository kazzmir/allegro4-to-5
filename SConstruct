import os, glob, re

debug = ARGUMENTS.get("debug", "")
static = ARGUMENTS.get("static", "")

def defaultEnvironment():
    env = Environment(ENV = os.environ)
    #env.Replace(CC = "clang")
    env.Append(CCFLAGS = ['-g3', '-Wall'])
    return env

def allegro_libname(name):
    allegro_version = "5.1"
    if static:
        version = "static-" + allegro_version
    else:
        version = allegro-version
    if debug:
        return name + "-debug-" + version
    else:
        return name + "-" + version

def CheckPKGConfig(context, version):
     context.Message( 'Checking for pkg-config... ' )
     ret = context.TryAction('pkg-config --atleast-pkgconfig-version=%s' % version)[0]
     context.Result( ret )
     return ret

def CheckPKG(context, name):
     context.Message( 'Checking for %s... ' % name )
     ret = context.TryAction('pkg-config --exists \'%s\'' % name)[0]
     context.Result( ret )
     return ret

class Cache:
    def __init__(self):
        self.allegro_libs = None

    def get_env(self):
        env = defaultEnvironment()

        if self.allegro_libs is None:
            self.allegro_libs = ["allegro", "allegro_primitives",
                "allegro_image", "allegro_font", "allegro_audio"]

            tests = {
                "CheckPKGConfig": CheckPKGConfig,
                "CheckPKG" : CheckPKG}
            conf = Configure(env, tests)
            if conf.CheckPKGConfig:
                if conf.CheckPKG(allegro_libname("allegro_monolith")):
                   self.allegro_libs = ["allegro_monolith"]
            env = conf.Finish()

        options = " --libs"
        if static:
            options += " --static"
        env.ParseConfig("pkg-config " + " ".join([allegro_libname(l)
            for l in self.allegro_libs]) + options)
        return env

cache = Cache()

def allegro4Environment():

    env = cache.get_env()

    env.Prepend(LIBS = [allegroLibrary(), 'm'])
    # env.Prepend(LIBS = [allegroLibrary()])
    env.Append(CPPPATH = ['#allegro4'])
    return env

allegro_store = [None]
def allegroLibrary():
    # Return the cached version
    if allegro_store[0] != None:
        return allegro_store[0]
    env = defaultEnvironment()
    library = SConscript('allegro4/SConscript', variant_dir = 'build/allegro', exports = ['env'], duplicate = False)
    Alias('library', library)
    #build = 'build-allegro'
    #env.VariantDir(build, 'allegro4')
    #source = Split("""allegro.c math3d.c math.c file.c unicode.c color.c clip3df.c""")
    #library = env.StaticLibrary('%s/allegro4-to-5' % build, ['%s/%s' % (build, file) for file in source])
    # Cache the allegro library so its not built each time allegroLibrary()
    # is called
    allegro_store[0] = library
    return allegro_store[0]

def makeExample(name, source):
    env = allegro4Environment()
    env.VariantDir('build/examples', 'examples')
    return env.Program(name, map(lambda x: 'build/examples/%s' % x, source))

def stars():
    return makeExample('exstars', ['exstars.c'])

def shade():
    return makeExample('exshade', ['exshade.c'])


def demos():
    env = allegro4Environment()
    SConscript('demos/SConscript', variant_dir = 'build/demos', exports = ['env'], duplicate = False)


examples = glob.glob("examples/*.c")
for example in examples:
    name = re.sub(r"examples/(.*?)\.c", r"\1", example)
    makeExample(name, [name + ".c"])

demos()
