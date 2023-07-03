#include "../Internal.h"

#include "../StbDummy.h"

#include <string.h>

void dflImageLoad(DflImage hImage)
{
    if(DFL_HANDLE(Image)->data != NULL) // no need to load an already loaded image
        return;

    DFL_HANDLE(Image)->data = stbi_load(DFL_HANDLE(Image)->path, &DFL_HANDLE(Image)->size.x, &DFL_HANDLE(Image)->size.y, 0, 4); //rgba channels

    if (DFL_HANDLE(Image)->data == NULL)
    {
        free(DFL_HANDLE(Image));
        return;
    }
}

DflImage dflImageFromFileGet(const char* file)
{
    struct DflImage_T* image = calloc(1, sizeof(struct DflImage_T));
    if(image == NULL)
        return NULL;

    strcpy_s(image->path, DFL_MAX_CHAR_COUNT, file);
    image->data = stbi_load(file, &image->size.x, &image->size.y, 0, 4); //rgba channels

    if(image->data == NULL)
    {
        free(image);
        return NULL;
    }

    return (DflImage)image;
}

DflImage dflImageReferenceFromFileGet(const char* file)
{
    struct DflImage_T* image = calloc(1, sizeof(struct DflImage_T));
    if (image == NULL)
        return NULL;

    strcpy_s(image->path, DFL_MAX_CHAR_COUNT, file);

    return (DflImage)image;
}

void dflImageDestroy(DflImage hImage)
{
   if(DFL_HANDLE(Image)->data != NULL)
        stbi_image_free(DFL_HANDLE(Image)->data);

    free(DFL_HANDLE(Image));
}
