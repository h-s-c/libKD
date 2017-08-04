/******************************************************************************
 * Copyright (c) 2010-2014 Emscripten authors, see AUTHORS file.
 * https://github.com/kripken/emscripten/blob/master/AUTHORS
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include <KD/kd.h>
#include <KD/kdext.h>
#include "test.h"

void create_file(const char *path, const char *buffer) 
{
    KDFile *file = kdFopen(path, "w");
    if(file == KD_NULL)
    {
        TEST_FAIL();
    }

    KDsize length = kdStrlen(buffer);
    KDsize retval = kdFwrite(buffer, length, 1, file);
    TEST_EQ(retval, length);

    kdFclose(file);
}

void setup() 
{
    create_file("file", "abcdef");
    kdMkdir("dir");
    create_file("dir/file", "abcdef");
    kdMkdir("dir/subdir");
    kdMkdir("dir-nonempty");
    kdMkdir("dir/subdir3");
    kdMkdir("dir/subdir3/subdir3_1");
    create_file("dir-nonempty/file", "abcdef");
}

void cleanup() 
{
    // we're hulk-smashing and removing original + renamed files to
    // make sure we get it all regardless of anything failing
    kdRemove("file");
    kdRemove("dir/file");
    kdRemove("dir/file1");
    kdRemove("dir/file2");
    kdRmdir("dir/subdir");
    kdRmdir("dir/subdir1");
    kdRmdir("dir/subdir2");
    kdRmdir("dir/subdir3/subdir3_1/subdir1 renamed");
    kdRmdir("dir/subdir3/subdir3_1");
    kdRmdir("dir/subdir3");
    kdRmdir("dir");
    kdRemove("dir-nonempty/file");
    kdRmdir("dir-nonempty");
}

void test() 
{
    KDint err;

    // can't rename something that doesn't exist
    err = kdRename("noexist", "dir");
    TEST_EQ(err, -1);
    TEST_EQ(kdGetError(), KD_ENOENT);

    // can't overwrite a folder with a file
    err = kdRename("file", "dir");
    TEST_EQ(err, -1);
#ifndef _MSC_VER
    TEST_EQ(kdGetError(), KD_EACCES);
#endif

    // can't overwrite a file with a folder
    err = kdRename("dir", "file");
    TEST_EQ(err, -1);
#ifndef _MSC_VER
    TEST_EQ(kdGetError(), KD_EINVAL);
#endif

    // can't overwrite a non-empty folder
    err = kdRename("dir", "dir-nonempty");
    TEST_EQ(err, -1);
#ifndef _MSC_VER
    TEST_EQ(kdGetError(), KD_EACCES);
#endif

    // source should not be ancestor of target
    err = kdRename("dir", "dir/somename");
    TEST_EQ(err, -1);
#ifndef _MSC_VER
    TEST_EQ(kdGetError(), KD_EINVAL);
#endif

    // target should not be an ancestor of source
    err = kdRename("dir/subdir", "dir");
    TEST_EQ(err, -1);
#ifndef _MSC_VER
    TEST_EQ(kdGetError(), KD_EACCES);
#endif

    // do some valid renaming
    err = kdRename("dir/file", "dir/file1");
    TEST_EQ(err, 0);
    err = kdRename("dir/file1", "dir/file2");
    TEST_EQ(err, 0);
    err = kdAccess("dir/file2", KD_R_OK);
    TEST_EQ(err, 0);
    err = kdRename("dir/subdir", "dir/subdir1");
    TEST_EQ(err, 0);
    err = kdRename("dir/subdir1", "dir/subdir2");
    TEST_EQ(err, 0);
    err = kdAccess("dir/subdir2", KD_R_OK);
    TEST_EQ(err, 0);

    err = kdRename("dir/subdir2", "dir/subdir3/subdir3_1/subdir1 renamed");
    TEST_EQ(err, 0);
    err = kdAccess("dir/subdir3/subdir3_1/subdir1 renamed", KD_R_OK);
    TEST_EQ(err, 0);
}

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    setup();
    test();
    cleanup();
    return 0;
}