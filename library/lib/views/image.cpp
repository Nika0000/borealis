/*
    Copyright 2019 WerWolv
    Copyright 2019 p-sam
    Copyright 2020-2021 natinusala

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <nanosvg.h>
#include <nanosvgrast.h>

#include <borealis/core/application.hpp>
#include <borealis/core/cache_helper.hpp>
#include <borealis/core/thread.hpp>
#include <borealis/core/util.hpp>
#include <borealis/views/image.hpp>
#include <cmath>
#include <fstream>
#include <limits>
#include <string_view>
#include <vector>

namespace brls
{

// Rasterizing at a higher resolution than the SVG's native size lets the image view
// upscale it without becoming too blurry, since the texture is otherwise sampled at 1:1.
static constexpr float SVG_RASTER_SCALE = 2.0f;
static constexpr float MAX_FILTER_RADIUS = 128.0f;

static const NVGfilter* getFilter(ImageFilterType type, float value, NVGfilter& filter)
{
    if (type == ImageFilterType::NONE)
        return nullptr;

    switch (type)
    {
        case ImageFilterType::GRAYSCALE:
            filter        = nvgFilterInit(NVG_FILTER_GRAYSCALE);
            filter.amount = value;
            break;
        case ImageFilterType::BRIGHTNESS:
            filter            = nvgFilterInit(NVG_FILTER_BRIGHTNESS_CONTRAST);
            filter.brightness = value;
            break;
        case ImageFilterType::CONTRAST:
            filter          = nvgFilterInit(NVG_FILTER_BRIGHTNESS_CONTRAST);
            filter.contrast = value;
            break;
        case ImageFilterType::BOX_BLUR:
            filter         = nvgFilterInit(NVG_FILTER_BOX_BLUR);
            filter.radiusX = std::isfinite(value) && value >= 0 && value <= MAX_FILTER_RADIUS ? (int) value : -1;
            filter.radiusY = filter.radiusX;
            break;
        case ImageFilterType::GAUSSIAN_BLUR:
            filter       = nvgFilterInit(NVG_FILTER_GAUSSIAN_BLUR);
            filter.sigma = value;
            break;
        case ImageFilterType::SHARPEN:
            filter        = nvgFilterInit(NVG_FILTER_SHARPEN);
            filter.amount = value;
            break;
        case ImageFilterType::UNSHARP_MASK:
            filter        = nvgFilterInit(NVG_FILTER_UNSHARP_MASK);
            filter.amount = value;
            break;
        case ImageFilterType::SOBEL:
            filter        = nvgFilterInit(NVG_FILTER_SOBEL);
            filter.amount = value;
            break;
        default:
            filter        = nvgFilterInit(NVG_FILTER_GRAYSCALE);
            filter.amount = -1;
            break;
    }

    return &filter;
}

static int createTextureFromPixels(unsigned char* data, int width, int height, int flags, const NVGfilter* filter)
{
    NVGcontext* vg = Application::getNVGContext();
    if (!filter)
        return nvgCreateImageRGBA(vg, width, height, flags, data);

    NVGpixelBuffer source = {};
    source.data           = data;
    source.width          = width;
    source.height         = height;

    int texture            = 0;
    NVGfilterStatus status = nvgCreateFilteredImageRGBA(vg, &source, flags, filter, 1, &texture);
    if (status != NVG_FILTER_OK)
        Logger::error("Cannot filter image: {}", (int) status);

    return texture;
}

static int createTextureFromSVG(const unsigned char* data, int size, int flags, const NVGfilter* filter)
{
    std::vector<char> buffer((const char*) data, (const char*) data + size);
    buffer.push_back('\0');

    NSVGimage* image = nsvgParse(buffer.data(), "px", 96.0f);
    if (!image)
        return 0;

    if (image->width <= 0 || image->height <= 0)
    {
        nsvgDelete(image);
        return 0;
    }

    int rasterWidth  = (int) std::ceil(image->width * SVG_RASTER_SCALE);
    int rasterHeight = (int) std::ceil(image->height * SVG_RASTER_SCALE);

    std::vector<unsigned char> pixels((size_t) rasterWidth * (size_t) rasterHeight * 4);

    NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
    nsvgRasterize(rasterizer, image, 0, 0, SVG_RASTER_SCALE, pixels.data(), rasterWidth, rasterHeight, rasterWidth * 4);
    nsvgDeleteRasterizer(rasterizer);
    nsvgDelete(image);

    return createTextureFromPixels(pixels.data(), rasterWidth, rasterHeight, flags, filter);
}

static int createTextureFromMem(const unsigned char* data, int size, int flags, const NVGfilter* filter)
{
    if (!data || size <= 0)
        return 0;

    std::string_view head((const char*) data, (size_t) std::min(size, 256));
    if (head.find("<svg") != std::string_view::npos)
        return createTextureFromSVG(data, size, flags, filter);

    if (!filter)
        return nvgCreateImageMem(Application::getNVGContext(), flags, const_cast<unsigned char*>(data), size);

    int texture = nvgCreateFilteredImageMem(Application::getNVGContext(), flags, data, size, filter, 1);
    if (texture == 0)
        Logger::error("Cannot decode or filter image");

    return texture;
}

static float measureWidth(
    YGNodeConstRef node,
    float width,
    YGMeasureMode widthMode,
    float height,
    YGMeasureMode heightMode,
    float originalWidth,
    ImageScalingType type
)
{
    if (widthMode == YGMeasureModeUndefined)
        return originalWidth;
    else if (widthMode == YGMeasureModeAtMost)
        if (type == ImageScalingType::FIT)
            return originalWidth;
        else
            return std::min(width, originalWidth);
    else if (widthMode == YGMeasureModeExactly)
        return width;
    else
        fatal("Unsupported Image width measure mode: " + std::to_string(widthMode));

    return width;
}

static float measureHeight(
    YGNodeConstRef node,
    float width,
    YGMeasureMode widthMode,
    float height,
    YGMeasureMode heightMode,
    float originalHeight,
    ImageScalingType type
)
{
    if (heightMode == YGMeasureModeUndefined)
        return originalHeight;
    else if (heightMode == YGMeasureModeAtMost)
        if (type == ImageScalingType::FIT)
            return originalHeight;
        else
            return std::min(height, originalHeight);
    else if (heightMode == YGMeasureModeExactly)
        return height;
    else
        fatal("Unsupported Image height measure mode: " + std::to_string(heightMode));

    return height;
}

static YGSize imageMeasureFunc(YGNodeConstRef node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode)
{
    Image* image                 = (Image*) YGNodeGetContext(node);
    int texture                  = image->getTexture();
    float originalWidth          = image->getOriginalImageWidth();
    float originalHeight         = image->getOriginalImageHeight();
    ImageScalingType scalingType = image->getScalingType();

    YGSize size = {
        .width  = std::isnan(width) ? 0.0f : width,
        .height = std::isnan(height) ? 0.0f : height,
    };

    if (texture == 0)
        return size;

    // Stretched mode: we don't care about the size of the image
    if (scalingType == ImageScalingType::STRETCH)
    {
        return size;
    }
    // Fit scaling mode: scale the view according to image ratio
    else if (scalingType == ImageScalingType::FIT && ntz(height) > 0)
    {
        float imageAspectRatio = originalWidth / originalHeight;

        // Grow height as much as possible then deduce width
        if (heightMode != YGMeasureModeUndefined)
        {
            if (ntz(width) > 0)
            {
                float viewAspectRatio = width / height;
                if (viewAspectRatio > imageAspectRatio)
                {
                    size.height = height;
                    size.width  = height * imageAspectRatio;
                }
                else
                {
                    size.width  = width;
                    size.height = width / imageAspectRatio;
                }
            }
            else
            {
                size.height = measureHeight(node, width, widthMode, height, heightMode, originalHeight, scalingType);
                size.width  = measureWidth(node, width, widthMode, height, heightMode, size.height * imageAspectRatio, scalingType);
            }
        }
        // Grow width as much as possible then deduce height
        else
        {
            size.width  = measureWidth(node, width, widthMode, height, heightMode, originalWidth, scalingType);
            size.height = measureHeight(node, width, widthMode, height, heightMode, size.width / imageAspectRatio, scalingType);
        }
    }
    // Crop (and fallback) method: grow as much as possible in both directions
    else
    {
        size.width  = measureWidth(node, width, widthMode, height, heightMode, originalWidth, scalingType);
        size.height = measureHeight(node, width, widthMode, height, heightMode, originalHeight, scalingType);
    }

    if (std::isnan(size.width))
        size.width = 0;
    if (std::isnan(size.height))
        size.height = 0;

    return size;
}

Image::Image()
{
    YGNodeSetMeasureFunc(this->ygNode, imageMeasureFunc);

    // This view uses a custom measure function, so the node type is automatically set to YGNodeTypeText,
    // which causes some deviations in the calculation of YGRoundToPixelGrid.
    // Another solution is to call `defaultConfig->setPointScaleFactor(factor)` in application.cpp.
    // (factor can be 0.0f or a larger value.)
    YGNodeSetNodeType(this->ygNode, YGNodeTypeDefault);

    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "scalingType", ImageScalingType, this->setScalingType,
        {
            { "fit", ImageScalingType::FIT },
            { "fill", ImageScalingType::FILL },
            { "stretch", ImageScalingType::STRETCH },
            { "center", ImageScalingType::CENTER },
        });

    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "imageAlign", ImageAlignment, this->setImageAlign,
        {
            { "top", ImageAlignment::TOP },
            { "right", ImageAlignment::RIGHT },
            { "bottom", ImageAlignment::BOTTOM },
            { "left", ImageAlignment::LEFT },
            { "center", ImageAlignment::CENTER },
        });

    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "interpolation", ImageInterpolation, this->setInterpolation,
        {
            { "linear", ImageInterpolation::LINEAR },
            { "nearest", ImageInterpolation::NEAREST },
        });

    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "filter", ImageFilterType, this->setFilter,
        {
            { "none", ImageFilterType::NONE },
            { "grayscale", ImageFilterType::GRAYSCALE },
            { "brightness", ImageFilterType::BRIGHTNESS },
            { "contrast", ImageFilterType::CONTRAST },
            { "boxBlur", ImageFilterType::BOX_BLUR },
            { "gaussianBlur", ImageFilterType::GAUSSIAN_BLUR },
            { "sharpen", ImageFilterType::SHARPEN },
            { "unsharpMask", ImageFilterType::UNSHARP_MASK },
            { "sobel", ImageFilterType::SOBEL },
        });

    this->registerFloatXMLAttribute("filterValue", [this](float value)
        { this->setFilter(this->filterType, value); });

    this->registerFilePathXMLAttribute(
        "image", [this](const std::string& value) { this->setImageFromFile(value); }

    );

    setClipsToBounds(true);
}

void Image::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    if (this->texture == 0)
        return;

    float coordX = x + this->imageX;
    float coordY = y + this->imageY;

    this->paint.xform[4] = coordX;
    this->paint.xform[5] = coordY;

    nvgBeginPath(vg);
    if (getClipsToBounds())
    {
        nvgRoundedRect(vg, x, y, width, height, getCornerRadius());
    }
    else
    {
        nvgRoundedRect(vg, coordX, coordY, this->imageWidth, this->imageHeight, getCornerRadius());
    }
    nvgFillPaint(vg, a(this->paint));
    nvgFill(vg);
}

void Image::onLayout() { this->invalidateImageBounds(); }

void Image::setImageAlign(ImageAlignment align)
{
    this->align = align;
    this->invalidateImageBounds();
}

void Image::invalidateImageBounds()
{
    if (this->texture == 0)
        return;

    float width  = this->getWidth();
    float height = this->getHeight();

    float viewAspectRatio  = height / width;
    float imageAspectRatio = this->originalImageHeight / this->originalImageWidth;

    switch (this->scalingType)
    {
        case ImageScalingType::FIT:
        {
            if (viewAspectRatio <= imageAspectRatio)
            {
                this->imageHeight = this->getHeight();
                this->imageWidth  = this->imageHeight / imageAspectRatio;
                this->imageX      = (width - this->imageWidth) / 2.0F;
                this->imageY      = 0;
            }
            else
            {
                this->imageWidth  = this->getWidth();
                this->imageHeight = this->imageWidth * imageAspectRatio;
                this->imageY      = (height - this->imageHeight) / 2.0F;
                this->imageX      = 0;
            }
            break;
        }
        case ImageScalingType::FILL:
        {
            if (viewAspectRatio >= imageAspectRatio)
            {
                this->imageHeight = this->getHeight();
                this->imageWidth  = this->imageHeight / imageAspectRatio;
                this->imageX      = (width - this->imageWidth) / 2.0F;
                this->imageY      = 0;
            }
            else
            {
                this->imageWidth  = this->getWidth();
                this->imageHeight = this->imageWidth * imageAspectRatio;
                this->imageY      = (height - this->imageHeight) / 2.0F;
                this->imageX      = 0;
            }
            break;
        }
        case ImageScalingType::STRETCH:
            this->imageX      = 0;
            this->imageY      = 0;
            this->imageWidth  = this->getWidth();
            this->imageHeight = this->getHeight();
            break;
        case ImageScalingType::CENTER:
            this->imageHeight = this->originalImageHeight;
            this->imageWidth  = this->originalImageWidth;
            this->imageX      = (width - this->imageWidth) / 2.0F;
            this->imageY      = (height - this->imageHeight) / 2.0F;
            break;
        default:
            fatal("Unimplemented Image scaling type");
    }

    // Create the paint - actual X and Y positions are updated every frame in draw() to apply translation (scrolling...)
    NVGcontext* vg = Application::getNVGContext();
    this->paint    = nvgImagePattern(vg, 0, 0, this->imageWidth, this->imageHeight, 0, this->texture, 1.0f);
}

size_t Image::checkCache(const std::string& path)
{
    int tex = brls::TextureCache::instance().getCache(path);
    if (tex > 0)
    {
        brls::Logger::verbose("cache hit: {} {}", path, tex);
        if (this->texture == tex)
            TextureCache::instance().removeCache(tex);
        else
            this->innerSetImage(tex);
        this->setFreeTexture(false);
        return tex;
    }

    return 0;
}

void Image::setImageFromRes(const std::string& name)
{
#ifdef USE_LIBROMFS
    NVGfilter filterStorage;
    const NVGfilter* filter = getFilter(this->filterType, this->filterValue, filterStorage);
    if (!filter && checkCache("@res/" + name) > 0)
        return;
    auto image = romfs::get(name);
    int tex    = createTextureFromMem((const unsigned char*) image.data(), (int) image.size(), this->getImageFlags(), filter);
    if (tex == 0)
        return;

    this->innerSetImage(tex);
    this->setFreeTexture(filter != nullptr);
    if (!filter)
        TextureCache::instance().addCache("@res/" + name, tex);
#else
    this->setImageFromFile(std::string(BRLS_RESOURCES) + name);
#endif
}

void Image::setInterpolation(ImageInterpolation interpolation) { this->interpolation = interpolation; }

void Image::setFilter(ImageFilterType filterType, float value)
{
    this->filterType  = filterType;
    this->filterValue = value;
}

void Image::clearFilter()
{
    this->filterType  = ImageFilterType::NONE;
    this->filterValue = 1.0f;
}

bool Image::hasFilter() const { return this->filterType != ImageFilterType::NONE; }

ImageFilterType Image::getFilterType() const { return this->filterType; }

float Image::getFilterValue() const { return this->filterValue; }

int Image::createImageFromRGBA(const unsigned char* data, int width, int height)
{
    if (!data || width <= 0 || height <= 0)
        return 0;

    NVGfilter filterStorage;
    const NVGfilter* filter = getFilter(this->filterType, this->filterValue, filterStorage);
    return createTextureFromPixels(const_cast<unsigned char*>(data), width, height, this->getImageFlags(), filter);
}

int Image::getImageFlags()
{
    if (this->interpolation == ImageInterpolation::NEAREST)
        return NVG_IMAGE_NEAREST;

    return 0;
}

void Image::setImageFromFile(const std::string& path)
{
    NVGfilter filterStorage;
    const NVGfilter* filter = getFilter(this->filterType, this->filterValue, filterStorage);
#ifdef USE_LIBROMFS
    if (path.rfind("@res/", 0) == 0)
        return this->setImageFromRes(path.substr(5));
#endif
    if (!filter && checkCache(path) > 0)
        return;

    int tex;
    if (endsWith(path, ".svg"))
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
        {
            Logger::error("Cannot open image file: {}", path);
            return;
        }

        std::streamsize size = file.tellg();
        if (size <= 0 || size > std::numeric_limits<int>::max())
            return;
        file.seekg(0, std::ios::beg);

        std::vector<unsigned char> data((size_t) size);
        if (!file.read((char*) data.data(), size))
            return;

        tex = createTextureFromSVG(data.data(), (int) data.size(), this->getImageFlags(), filter);
    }
    else if (filter)
    {
        tex = nvgCreateFilteredImage(Application::getNVGContext(), path.c_str(), this->getImageFlags(), filter, 1);
    }
    else
    {
        tex = nvgCreateImage(Application::getNVGContext(), path.c_str(), this->getImageFlags());
    }

    if (tex == 0)
        return;

    this->innerSetImage(tex);
    this->setFreeTexture(filter != nullptr);

    if (!filter)
        TextureCache::instance().addCache(path, tex);
}

void Image::setImageFromMem(const unsigned char* data, int size)
{
    NVGfilter filterStorage;
    const NVGfilter* filter = getFilter(this->filterType, this->filterValue, filterStorage);
    int tex                 = createTextureFromMem(data, size, this->getImageFlags(), filter);
    if (tex == 0)
        return;

    this->innerSetImage(tex);
    this->setFreeTexture(true);
}

void Image::setImageAsync(const std::function<void(std::function<void(const std::string&, size_t length)>)>& cb)
{
    ASYNC_RETAIN
    cb(
        [ASYNC_TOKEN](const std::string& data, size_t length)
        {
            brls::sync(
                [ASYNC_TOKEN, data, length]()
                {
                    ASYNC_RELEASE
                    if (length == 0)
                        return;
                    this->setImageFromMem((unsigned char*) data.c_str(), (int) length);
                }
            );
        }
    );
}

void Image::innerSetImage(int tex)
{
    if (tex == 0)
    {
        Logger::error("Cannot set texture: 0");
        return;
    }

    NVGcontext* vg = Application::getNVGContext();

    // Free the old texture if necessary
    if (this->texture != 0 && this->texture != tex)
    {
        if (this->freeTexture)
            nvgDeleteImage(vg, this->texture);
        else
            TextureCache::instance().removeCache(this->texture);
    }

    // Set the new texture
    this->texture = tex;

    int width, height;
    nvgImageSize(vg, this->texture, &width, &height);
    this->originalImageWidth  = (float) width;
    this->originalImageHeight = (float) height;

    this->invalidate();
}

void Image::clear()
{
    if (this->texture == 0)
        return;

    if (this->freeTexture)
        nvgDeleteImage(Application::getNVGContext(), this->texture);
    else
        TextureCache::instance().removeCache(this->texture);

    this->texture             = 0;
    this->originalImageWidth  = 0;
    this->originalImageHeight = 0;
    this->imageX              = 0;
    this->imageY              = 0;
    this->imageWidth          = 0;
    this->imageHeight         = 0;
    this->freeTexture         = true;

    this->invalidate();
}

void Image::setScalingType(ImageScalingType scalingType)
{
    this->scalingType = scalingType;

    this->invalidate();
}

ImageScalingType Image::getScalingType() { return this->scalingType; }

float Image::getOriginalImageWidth() const { return this->originalImageWidth; }

int Image::getTexture() const { return this->texture; }

void Image::setFreeTexture(bool value) { this->freeTexture = value; }

bool Image::getFreeTexture() const { return this->freeTexture; }

float Image::getOriginalImageHeight() const { return this->originalImageHeight; }

Image::~Image()
{
    if (this->freeTexture && this->texture != 0)
        nvgDeleteImage(Application::getNVGContext(), this->texture);
    else
        TextureCache::instance().removeCache(this->texture);
}

View* Image::create() { return new Image(); }

} // namespace brls
