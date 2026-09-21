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

#pragma once

#include <borealis/core/view.hpp>

namespace brls
{

// This dictates what to do with the image if there is not
// enough room for the view to grow and display the whole image,
// or if the view is bigger than the image
enum class ImageScalingType
{
    // The image is scaled to fit the view boundaries, aspect ratio is conserved
    FIT,
    // The image is scaled to fill the view boundaries, aspect ratio is conserved
    FILL,
    // The image is stretched to fit the view boundaries (aspect ratio is not conserved). The original image dimensions are entirely ignored
    // in the layout process.
    STRETCH,
    // The image is either cropped (not enough space) or untouched (too much space)
    CENTER,
};

// This dictates what interpolation to use when down / up scaling the image
enum class ImageInterpolation
{
    LINEAR,
    NEAREST
};

// Built-in filter presets available through the filter XML attribute
enum class ImageFilterType
{
    NONE,
    GRAYSCALE,
    BRIGHTNESS,
    CONTRAST,
    BOX_BLUR,
    GAUSSIAN_BLUR,
    SHARPEN,
    UNSHARP_MASK,
    SOBEL,
};

// Alignment of the image inside the view for FIT and CROP scaling types
enum class ImageAlignment
{
    TOP,
    RIGHT,
    BOTTOM,
    LEFT,
    CENTER,
};

// An image. The view will try to grow as much
// as possible to fit the image. The scaling type dictates
// what to do with the image if there is not enough or too much space
// for the view compared to the image inside.
// Supported formats are: JPG, PNG, TGA, BMP and GIF (not animated).
class Image : public View
{
  public:
    Image();
    ~Image();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    void onLayout() override;

    /**
     * Sets the interpolation method for the image. Default is LINEAR.
     *
     * As the interpolation is set when the texture is created, and since we don't
     * want to always store the image buffer in the view, this only takes effect
     * after (re) loading the image using the setImage* methods.
     *
     * If you are using the interpolation XML attribute, you have to set it before the
     * actual image attribute.
     */
    void setInterpolation(ImageInterpolation interpolation);

    /**
     * Sets a filter for subsequent image loads. Value is grayscale strength [0, 1],
     * brightness offset [-1, 1], contrast [0, 10000], box-blur radius [0, 128],
     * Gaussian sigma (0, 42], or effect strength [0, 10000], depending on type.
     * Filtering runs once while loading and does not affect drawing. Filtered
     * images are not cached and no source pixels or extra texture are retained.
     * In XML, order attributes as filter, filterValue, then image.
     */
    void setFilter(ImageFilterType filterType, float value = 1.0f);

    /**
     * Disables filtering for subsequent image loads.
     */
    void clearFilter();

    /**
     * Returns whether subsequent image creation should apply a filter.
     */
    bool hasFilter() const;

    /**
     * Returns the selected built-in filter type, or NONE if filtering is disabled.
     */
    ImageFilterType getFilterType() const;

    /**
     * Returns the configured value for the selected filter.
     */
    float getFilterValue() const;

    /**
     * Creates a NanoVG texture from decoded RGBA pixels, applying the configured
     * filter and interpolation settings. The caller owns the returned texture.
     */
    int createImageFromRGBA(const unsigned char* data, int width, int height);

    /**
     * Sets the image from the given resource name.
     *
     * See Image class documentation for the list of supported
     * image formats.
     */
    void setImageFromRes(const std::string& name);

    /**
     * Sets the image from the given file path.
     *
     * See Image class documentation for the list of supported
     * image formats.
     */
    void setImageFromFile(const std::string& path);

    /**
     * Sets the image from memory.
     *
     * See Image class documentation for the list of supported
     * image formats.
     */
    void setImageFromMem(const unsigned char* data, int size);

    virtual void innerSetImage(int texture);

    void setImageAsync(const std::function<void(std::function<void(const std::string&, size_t length)>)>& cb);

    void clear();

    /**
     * Sets the scaling type for this image.
     *
     * Default is FIT.
     */
    void setScalingType(ImageScalingType scalingType);

    ImageScalingType getScalingType();

    /**
     * Sets the alignment of the image inside the view.
     * Only used if scaling it set to FIT when the aspect ratio is different
     * or CROP, to change the region of the image to draw.
     *
     * Default is CENTER.
     */
    void setImageAlign(ImageAlignment align);

    /**
     * Whether to destroy the current texture before updating the image texture
     */
    void setFreeTexture(bool value);
    bool getFreeTexture() const;

    int getTexture() const;
    float getOriginalImageWidth() const;
    float getOriginalImageHeight() const;

    static View* create();

  protected:
    ImageScalingType scalingType     = ImageScalingType::FIT;
    ImageAlignment align             = ImageAlignment::CENTER;
    ImageInterpolation interpolation = ImageInterpolation::LINEAR;

    int texture = 0;

    NVGpaint paint;

    ImageFilterType filterType = ImageFilterType::NONE;
    float filterValue          = 1.0f;

    void invalidateImageBounds();
    int getImageFlags();
    size_t checkCache(const std::string& path);

    float originalImageWidth  = 0;
    float originalImageHeight = 0;

    float imageX      = 0;
    float imageY      = 0;
    float imageHeight = 0;
    float imageWidth  = 0;

    bool freeTexture = true;
};

} // namespace brls
