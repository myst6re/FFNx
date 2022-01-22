
#include <string>
#include <map>

class TexturePacker {
public:
    enum Depth {
        Indexed4Bit = 0,
        Indexed8Bit,
        R5B5G5
    };
    TexturePacker(uint8_t *vram, int w, int h, Depth depth = R5B5G5);
    void setTexture(const std::string &name, const uint8_t *texture, int x, int y, int w, int h, Depth depth = R5B5G5);
    void getTexture(uint8_t *texture, int x, int y, int w, int h, Depth depth = R5B5G5);
    void copyTexture(int sourceX, int sourceY, int targetX, int targetY, int w, int h, Depth depth = R5B5G5);
    void fill(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, Depth depth = R5B5G5);

    std::string textureNameFromInfos(int x, int y, int w, int h, Depth depth);

    bool saveVram(const char *fileName);
private:
    inline uint8_t *vram_seek(int x, int y) const {
        return _vram + _depth * (x + y * _w);
    }

    void vramToR8G8B8(uint32_t *output);
    static inline uint32_t fromR5G5B5Color(uint16_t color) {
        uint8_t r = color & 31,
                g = (color >> 5) & 31,
                b = (color >> 10) & 31;

        return (0xffu << 24) |
            ((((r << 3) + (r >> 2)) & 0xffu) << 16) |
            ((((g << 3) + (g >> 2)) & 0xffu) << 8) |
            (((b << 3) + (b >> 2)) & 0xffu);
    }
    class Texture {
    public:
        Texture(
            const std::string &name,
            int x, int y, int w, int h,
            TexturePacker::Depth depth
        );
    private:
        std::string _name;
        int _x, _y;
        int _w, _h;
        TexturePacker::Depth _depth;
    };

    uint8_t *_vram;
    int _w, _h;
    Depth _depth;
    std::map<std::string, Texture> _textures;
};
