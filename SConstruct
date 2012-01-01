import os, glob, re

debug = ARGUMENTS.get("debug", "")

def defaultEnvironment():
    env = Environment(ENV = os.environ)
    env.Append(CCFLAGS = ['-g3', '-Wall'])
    return env

def allegro4Environment():
    allegro_libs = ["allegro", "allegro_primitives", "allegro_image", "allegro_font"]
    allegro_version = "5.1"
    env = defaultEnvironment()
    if debug:
        env.ParseConfig("pkg-config " + " ".join([l + "-debug-" + allegro_version for l in allegro_libs]) + " --libs")
    else:
        env.ParseConfig("pkg-config " + " ".join([l + "-" + allegro_version for l in allegro_libs]) + " --libs")
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
