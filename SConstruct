import os

def defaultEnvironment():
    env = Environment(ENV = os.environ)
    env.Append(CCFLAGS = ['-g3'])
    return env

allegro_store = [None]
def allegroLibrary():
    if allegro_store[0] != None:
        return allegro_store[0]
    env = defaultEnvironment()
    env.VariantDir('build-allegro', 'allegro4')
    library = env.StaticLibrary(Split("""
build-allegro/allegro.c
build-allegro/math3d.c
build-allegro/math.c
build-allegro/file.c
build-allegro/unicode.c
"""))
    allegro_store[0] = library
    return allegro_store[0]

def makeExample(name, source):
    env = defaultEnvironment()
    env.ParseConfig('pkg-config allegro-5.1 allegro_primitives-5.1 allegro_image-5.1 --cflags --libs')
    env.VariantDir('build-examples', 'examples')
    env.Append(LIBS = [allegroLibrary()])
    # env.Prepend(LIBS = [allegroLibrary()])
    env.Append(CPPPATH = ['allegro4'])
    return env.Program(name, map(lambda x: 'build-examples/%s' % x, source))

def stars():
    return makeExample('exstars', ['exstars.c'])

def shade():
    return makeExample('exshade', ['exshade.c'])
    
stars()
shade()
