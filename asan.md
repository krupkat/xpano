Rewrite one line in opencv to get rid of libconcrt.lib dependency.

The [Concurrency runtime](https://docs.microsoft.com/en-us/cpp/parallel/concrt/concurrency-runtime?view=msvc-170) cannot be linked to an asan istrumented binary.

```
diff --git a/modules/core/src/parallel.cpp b/modules/core/src/parallel.cpp
index 26e5e8c..d95bae4 100644
--- a/modules/core/src/parallel.cpp
+++ b/modules/core/src/parallel.cpp
@@ -85,7 +85,7 @@
 #endif

 #if defined _MSC_VER && _MSC_VER >= 1600
-    #define HAVE_CONCURRENCY
+    #undef HAVE_CONCURRENCY
 #endif
```
