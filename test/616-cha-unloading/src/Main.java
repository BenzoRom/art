import java.lang.ref.WeakReference;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;

public class Main {
  static final String DEX_FILE = System.getenv("DEX_LOCATION") + "/616-cha-unloading-ex.jar";
  static final String LIBRARY_SEARCH_PATH = System.getProperty("java.library.path");
  static Constructor<?> sConstructor;

  private static class CHAUnloaderRetType {
    public CHAUnloaderRetType(WeakReference<ClassLoader> cl,
                              AbstractCHATester obj,
                              long methodPtr) {
      this.cl = cl;
      this.obj = obj;
      this.methodPtr = methodPtr;
    }
    public WeakReference<ClassLoader> cl;
    public AbstractCHATester obj;
    public long methodPtr;
  }

  public static void main(String[] args) throws Exception {
    System.loadLibrary(args[0]);

    Class<?> pathClassLoader = Class.forName("dalvik.system.PathClassLoader");
    if (pathClassLoader == null) {
        throw new AssertionError("Couldn't find path class loader class");
    }
    sConstructor =
        pathClassLoader.getDeclaredConstructor(String.class, String.class, ClassLoader.class);

    try {
        testUnload();
    } catch (Exception e) {
        e.printStackTrace();
    }
  }

  private static void testUnload() throws Exception {
    // Load a concrete class, then unload it. Get a deleted ArtMethod to test if it'll be inlined.
    CHAUnloaderRetType result = doUnloadLoader();
    WeakReference<ClassLoader> loader = result.cl;
    long methodPtr = result.methodPtr;
    // Check that the classloader is indeed unloaded.
    System.out.println(loader.get());

    // Reuse the linear alloc so old pointers so it becomes invalid.
    boolean ret = tryReuseArenaOfMethod(methodPtr, 10);
    // Check that we indeed reused it.
    System.out.println(ret);

    // Try to JIT-compile under dangerous conditions.
    ensureJitCompiled(Main.class, "targetMethodForJit");
    System.out.println("Done");
  }

  private static void doUnloading() {
    // Do multiple GCs to prevent rare flakiness if some other thread is keeping the
    // classloader live.
    for (int i = 0; i < 5; ++i) {
       Runtime.getRuntime().gc();
    }
  }

  private static CHAUnloaderRetType setupLoader()
      throws Exception {
    ClassLoader loader = (ClassLoader) sConstructor.newInstance(
        DEX_FILE, LIBRARY_SEARCH_PATH, ClassLoader.getSystemClassLoader());
    Class<?> concreteCHATester = loader.loadClass("ConcreteCHATester");

    // Preemptively compile methods to prevent delayed JIT tasks from blocking the unloading.
    ensureJitCompiled(concreteCHATester, "<init>");
    ensureJitCompiled(concreteCHATester, "lonelyMethod");

    Object obj = concreteCHATester.newInstance();
    Method lonelyMethod = concreteCHATester.getDeclaredMethod("lonelyMethod", (Class<?>[])null);

    // Get a pointer to a region that shall be not used after the unloading.
    long artMethod = getArtMethod(lonelyMethod);

    AbstractCHATester ret = null;
    return new CHAUnloaderRetType(new WeakReference(loader), ret, artMethod);
  }

  private static CHAUnloaderRetType targetMethodForJit(int mode)
      throws Exception {
    CHAUnloaderRetType ret = new CHAUnloaderRetType(null, null, 0);
    if (mode == 0) {
      ret = setupLoader();
    } else if (mode == 1) {
      // This brach is not supposed to be executed. It shall trigger "lonelyMethod" inlining
      // during jit compilation of "targetMethodForJit".
      ret = setupLoader();
      AbstractCHATester obj = ret.obj;
      obj.lonelyMethod();
    }
    return ret;
  }

  private static CHAUnloaderRetType doUnloadLoader()
      throws Exception {
    CHAUnloaderRetType result = targetMethodForJit(0);
    doUnloading();
    return result;
  }

  public static native void ensureJitCompiled(Class<?> itf, String method_name);
  public static native long getArtMethod(Object javaMethod);
  public static native boolean tryReuseArenaOfMethod(long artMethod, int tries_count);
}
