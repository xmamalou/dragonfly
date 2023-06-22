#include "../Internal.h"

#include "../StbDummy.h"

DflImage dflImageFromFileGet(const char* path)
{
    struct DflImage_T* image = calloc(1, sizeof(struct DflImage_T));
    if(image == NULL)
        return NULL;

    image->data = stbi_load(path, &image->size.x, &image->size.y, 0, 4); //rgba channels

    if(image->data == NULL)
    {
        free(image);
        return NULL;
    }

    return (DflImage)image;
}