/*
 * Copyright 2009 AOL LLC 
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at 
 *         
 *         http://www.apache.org/licenses/LICENSE-2.0 
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#include "mod_dims.h"

#define MAGICK_CHECK(func, rec) \
    do { \
        apr_status_t code = func; \
        if(rec->status == DIMS_IMAGEMAGICK_TIMEOUT) {\
            return DIMS_IMAGEMAGICK_TIMEOUT; \
        } else if(code == MagickFalse) {\
            return DIMS_FAILURE; \
        } \
    } while(0)

/*
apr_status_t
dims_smart_crop_operation (dims_request_rec *d, char *args, char **err) {
    MagickStatusType flags;
    RectangleInfo rec;
    ExceptionInfo ex_info;

    flags = ParseGravityGeometry(GetImageFromMagickWand(d->wand), args, &rec, &ex_info);
    if(!(flags & AllValues)) {
        *err = "Parsing crop geometry failed";
        return DIMS_FAILURE;
    }

    // MAGICK_CHECK(MagickResizeImage(d->wand, rec.width, rec.height, UndefinedFilter, 1), d);
    smartCrop(d->wand, 20, rec.width, rec.height);

    return DIMS_SUCCESS;
}
*/

apr_status_t
dims_strip_operation (dims_request_rec *d, char *args, char **err) {

    /* If args is passed from the user and 
     *   a) it equals true, strip the image.
     *   b) it equals false, don't strip the image.
     *   c) it is neither true/false, strip based on config value.
     * If args is NULL, strip based on config value.
     */
    if(args != NULL) {
        if(strcmp(args, "true") == 0 || ( strcmp(args, "false") != 0 && d->config->strip_metadata )) {
            MAGICK_CHECK(MagickStripImage(d->wand), d);
        }
    }
    else if(d->config->strip_metadata) {
        MAGICK_CHECK(MagickStripImage(d->wand), d);
    }

    return DIMS_SUCCESS;
}

apr_status_t
dims_resize_operation (dims_request_rec *d, char *args, char **err) {
    MagickStatusType flags;
    RectangleInfo rec;

    rec.width = MagickGetImageWidth(d->wand);
    rec.height = MagickGetImageHeight(d->wand);

    flags = ParseMetaGeometry(args, &rec.x, &rec.y, &rec.width, &rec.height);
    if(!(flags & AllValues)) {
        *err = "Parsing thumbnail geometry failed";
        return DIMS_FAILURE;
    }

    MAGICK_CHECK(MagickScaleImage(d->wand, rec.width, rec.height), d);

    return DIMS_SUCCESS;
}

apr_status_t
dims_sharpen_operation (dims_request_rec *d, char *args, char **err) {
    MagickStatusType flags;
    GeometryInfo geometry;

    flags = ParseGeometry(args, &geometry);
    if ((flags & SigmaValue) == 0) {
        geometry.sigma=1.0;
    }

    MAGICK_CHECK(MagickSharpenImage(d->wand, geometry.rho, geometry.sigma), d);

    return DIMS_SUCCESS;
}

apr_status_t
dims_unsharp_operation (dims_request_rec *d, char *args, char **err) {
    if(strcmp( args, "true") == 0) {
        MAGICK_CHECK(MagickUnsharpMaskImage(d->wand, 1.2, 0.6, 4.0, 0), d);
    }
    return DIMS_SUCCESS;
}

apr_status_t
dims_thumbnail_operation (dims_request_rec *d, char *args, char **err) {
    MagickStatusType flags;
    RectangleInfo thumb_info, crop_info;
    char *resize_args = apr_psprintf(d->pool, "%s^", args);

    thumb_info.width = MagickGetImageWidth(d->wand);
    thumb_info.height = MagickGetImageHeight(d->wand);

    flags = ParseMetaGeometry(resize_args, &thumb_info.x, &thumb_info.y, &thumb_info.width, &thumb_info.height);
    if(!(flags & AllValues)) {
        *err = "Parsing thumbnail (resize) geometry failed";
        return DIMS_FAILURE;
    }

    MAGICK_CHECK(MagickThumbnailImage(d->wand, thumb_info.width, thumb_info.height), d);

    if(!(flags & PercentValue)) {
        flags = ParseAbsoluteGeometry(args, &crop_info);
        if(!(flags & AllValues)) {
            *err = "Parsing thumbnail (crop) geometry failed";
            return DIMS_FAILURE;
        }

        MAGICK_CHECK(MagickCropImage(d->wand, crop_info.width, crop_info.height,
        (int)((thumb_info.width-crop_info.width)/2),
        (int)((thumb_info.height-crop_info.height)/2)), d);
    }
    
    return DIMS_SUCCESS;
}

apr_status_t
dims_crop_operation (dims_request_rec *d, char *args, char **err) {
    MagickStatusType flags;
    RectangleInfo rec;
    ExceptionInfo ex_info;

    flags = ParseGravityGeometry(GetImageFromMagickWand(d->wand), args, &rec, &ex_info);
    if(!(flags & AllValues)) {
        *err = "Parsing crop geometry failed";
        return DIMS_FAILURE;
    }

    MAGICK_CHECK(MagickCropImage(d->wand, rec.width, rec.height, rec.x, rec.y), d);

    return DIMS_SUCCESS;
}

apr_status_t
dims_format_operation (dims_request_rec *d, char *args, char **err) {
    MAGICK_CHECK(MagickSetFormat(d->wand, args), d);
    return DIMS_SUCCESS;
}

apr_status_t
dims_quality_operation (dims_request_rec *d, char *args, char **err) {
    int quality = apr_strtoi64(args, NULL, 0);
    int existing_quality = MagickGetImageCompressionQuality(d->wand);

    if(quality < existing_quality) {
        MAGICK_CHECK(MagickSetImageCompressionQuality(d->wand, quality), d);
    }
    return DIMS_SUCCESS;
}
