import os

def defaultEnvironment():
    env = Environment(ENV = os.environ)
    env.Append(CCFLAGS = ['-g3', '-Wall'])
    return env

def allegro4Environment():
    env = defaultEnvironment()
    env.ParseConfig('pkg-config allegro-5.1 allegro_primitives-5.1 allegro_image-5.1 allegro_font-5.1 --cflags --libs')
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
    library = SConscript('allegro4/SConscript', variant_dir = 'build-allegro', exports = ['env'])
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
    env.VariantDir('build-examples', 'examples')
    return env.Program(name, map(lambda x: 'build-examples/%s' % x, source))

def stars():
    return makeExample('exstars', ['exstars.c'])

def shade():
    return makeExample('exshade', ['exshade.c'])

def bitmap():
    return makeExample('exbitmap', ['exbitmap.c'])

def keys():
    return makeExample('exkeys', ['exkeys.c'])

def camera():
    return makeExample('excamera', ['excamera.c'])

def mouse():
    return makeExample('exmouse', ['exmouse.c'])

def config():
    return makeExample('exconfig', ['exconfig.c'])

def data():
    return makeExample('exdata', ['exdata.c'])

stars()
shade()
bitmap()
keys()
camera()
mouse()
config()
#data()
