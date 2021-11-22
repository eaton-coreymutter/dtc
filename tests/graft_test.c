#include <stdio.h>
#include "libfdt.h"
#include "tests.h"
#include "testdata.h"

static void check_prop_path(void *fdt, const char *node_path, const char *prop_name, 
    unsigned int exp_size, void *exp_val)
{
    int node = fdt_path_offset(fdt, node_path);
    if (node < 0)
    {
        FAIL("Looking for node %s: %s", node_path, fdt_strerror(node));
    }
    check_property(fdt, node, prop_name, exp_size, exp_val);
}

static void *src_setup(void)
{
    void *fdt;
    int parent, prop;

    fdt = xmalloc(4096);
    fdt_create_empty_tree(fdt, 2048);
    parent = fdt_add_subnode(fdt, 0, "a");
    if (parent < 0)
    {
        FAIL("Setup error %s adding /a", fdt_strerror(parent));
    }
    prop = fdt_appendprop_u32(fdt, parent, "b", 27);
    if (prop < 0)
    {
        FAIL("Setup error %s adding /a/b", fdt_strerror(prop));
    }
    return fdt;
}


static void node_path_test(void *fdt)
{
    int node, prop;
    uint32_t exp_val;

    node = fdt_create_node_path(fdt, "/x/y/z/foo");
    if (node < 0)
    {
        FAIL("Error creating node_path /x/y/z/foo: %s", fdt_strerror(node));
    }
    prop = fdt_appendprop_u32(fdt, node, "bar", 55);
    if (prop < 0)
    {
        FAIL("Error appending property bar: %s", fdt_strerror(prop));
    }
    exp_val = cpu_to_fdt32(55);
    check_prop_path(fdt, "/x/y/z/foo", "bar", 4, &exp_val);
}

static void copy_test_1(void *fdt)
{
    int node, prop, rc, size;
    void *dst_fdt = malloc(4096);
    uint32_t exp_val = cpu_to_fdt32(27);

    fdt_create_empty_tree(dst_fdt, 2048);
    node = fdt_create_node_path(dst_fdt, "/foo/bar");
    if (node < 0)
    {
        FAIL("Error creating node_path /foo/bar: %s", fdt_strerror(node));
    }
    prop = fdt_appendprop_string(dst_fdt, node, "baz", "TESTING");
    if (prop < 0)
    {
        FAIL("Error setting prop baz: %s", fdt_strerror(prop));
    }
    check_prop_path(dst_fdt, "/foo/bar", "baz", 8, "TESTING");
    rc = fdt_path_offset(dst_fdt, "/a");
    if (rc != -FDT_ERR_NOTFOUND)
    {
        FAIL("Expected not-found on /a, got %d", rc);
    }
    size = fdt_totalsize(dst_fdt);
    if (size != 2048)
    {
        FAIL("Total size of dst_fdt expected 2048, got %d", size);
    }
    rc = fdt_copy_tree(fdt, dst_fdt, "/a");
    if (rc < 0)
    {
        FAIL("Error %s on fdt_copy_tree", fdt_strerror(rc));
    }
    check_prop_path(dst_fdt, "/foo/bar", "baz", 8, "TESTING");
    check_prop_path(dst_fdt, "/a", "b", 4, &exp_val);
    size = fdt_totalsize(dst_fdt);
    if (size <= 2048)
    {
        FAIL("Expected dst FDT to grow, size is %d instead", fdt_totalsize(dst_fdt));
    }
    if (size > 4096)
    {
        FAIL("dst_fdt grew too much, to %d", size);
    }
    free(dst_fdt);
}

static void merge_firmware_reserved_memory(const char *a, const char *b, const char *dst)
{
    void *src_fdt, *dst_fdt;
    int rc;

    src_fdt = load_blob(a);
    dst_fdt = load_blob(b);
    rc = fdt_copy_tree(src_fdt, dst_fdt, "/firmware");
    if (rc < 0)
    {
        FAIL("Error copying /firmware: %s", fdt_strerror(rc));
    }
    rc = fdt_copy_tree(src_fdt, dst_fdt, "/reserved-memory");
    if (rc < 0)
    {
        FAIL("Error copying /reserved-memory: %s", fdt_strerror(rc));
    }
    save_blob(dst, dst_fdt);
}


int main(int argc, char *argv[])
{
    void *src_fdt;
    uint32_t exp_val;

    test_init(argc, argv);

    src_fdt = src_setup();

    exp_val = cpu_to_fdt32(27);
    check_prop_path(src_fdt, "/a", "b", 4, &exp_val);

    node_path_test(src_fdt);

    copy_test_1(src_fdt);

    free(src_fdt);

    if (argc == 4)
    {
        merge_firmware_reserved_memory(argv[1], argv[2], argv[3]);
    }

    PASS();
}
