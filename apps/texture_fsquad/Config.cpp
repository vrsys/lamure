class Config
{
public:
    // Sections
    static constexpr const char *TEXTURE_MANAGEMENT = "TEXTURE_MANAGEMENT";
    static constexpr const char *DEBUG = "DEBUG";

    // Texture management fields
    static constexpr const char *TILE_SIZE = "TILE_SIZE";
    static constexpr const char *FILE_PPM = "FILE_PPM";
    static constexpr const char *FILE_MIPMAP = "FILE_MIPMAP";

    // Debug fields
    static constexpr const char *KEEP_INTERMEDIATE_DATA = "KEEP_INTERMEDIATE_DATA";

    static constexpr const char *DEFAULT = "DEFAULT";
};