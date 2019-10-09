if (CONTAINER_DEPLOYMENT_BUILD)

set(ImageMagick_EXECUTABLE_DIR, ${GLOBAL_EXT_DIR}/imagemagick)

set(ImageMagick_Magick++_INCLUDE_DIR, ${GLOBAL_EXT_DIR}/imagemagick/include)
set(ImageMagick_MagickCore_INCLUDE_DIR, ${GLOBAL_EXT_DIR}/imagemagick/include)
set(ImageMagick_MagickWand_INCLUDE_DIR, ${GLOBAL_EXT_DIR}/imagemagick/include)

set(ImageMagick_Magick++_LIBRARY, ${GLOBAL_EXT_DIR}/imagemagick/lib/CORE_RL_Magick++_.lib)
set(ImageMagick_MagickCore_LIBRARY, ${GLOBAL_EXT_DIR}/imagemagick/lib/CORE_RL_MagickCore_.lib)
set(ImageMagick_MagickWand_LIBRARY, ${GLOBAL_EXT_DIR}/imagemagick/lib/CORE_RL_MagickWand_.lib)

string(CONCAT ImageMagick_INCLUDE_DIRS ${ImageMagick_Magick++_INCLUDE_DIR} ${ImageMagick_MagickCore_INCLUDE_DIR} ${ImageMagick_MagickWand_INCLUDE_DIR})
string(CONCAT ImageMagick_LIBRARIES ${ImageMagick_Magick++_LIBRARY} ${ImageMagick_MagickCore_LIBRARY} ${ImageMagick_MagickWand_LIBRARY})

set(ImageMagick_mogrify_EXECUTABLE, ${GLOBAL_EXT_DIR}/imagemagick/mogrify.exe)
set(ImageMagick_FOUND, TRUE) #sort of

endif(CONTAINER_DEPLOYMENT_BUILD)