/* SPDX-License-Identifier: GPL-3.0-only */
/* The style library packed for the FM-1's flash: every profile of tuna_style.c (175 of them, 444 bytes each, 78 KB as
 * a table) as the bytes in which it differs from a base, the commonest value of each field; tuna_style.c expands one
 * when it is asked for (TUNA_PACKED_STYLES, the firmware and the browser build). Exact: each value is stored as the
 * bytes the table holds, a float as its bits.
 *   build/host/tuna_style_pack firmware/src/tuna_styles_packed.inc           write it
 *   build/host/tuna_style_pack --check firmware/src/tuna_styles_packed.inc   1 when it is not up to date
 *   build/host/tuna_style_pack --print                                       every style's bytes, field by field
 * (--print by the packed build too: tests/tuna_tests.sh compares the two.)
 * The format: per style, entries of a field number, an element index and the element's bytes (as many as its kind
 * takes), ended by 0xFF; the fields of TG_FIELDS, then TGP_ORCH .. TGP_DRUM for what manners do not change. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TUNA_STYLE_PACKER 1
#include "../firmware/src/tuna_style.c"

/* (the four fields TG_FIELDS leaves to the manners' own sets: tuna_style.c's TGP_EXTRA_FIELDS, EXTRA here) */

static int ksize(int kind)
{
    switch (kind) {
    case TGK_F32: case TGK_I32: case TGK_U32: return 4;
    case TGK_U16: case TGK_I16: return 2;
    default: return 1;
    }
}
static const tg_field *field(int k) { return k < TGF_COUNT ? &TG_FIELDS[k] : &EXTRA[k - TGF_COUNT]; }
#define NF (TGF_COUNT + TGP_EXTRA)

/* --print: field by field, element by element, the bytes in hex: the same text from the table and from the packing */
static void print_all(void)
{
    for (int s = 0; s < TG_STYLE_COUNT; ++s) {
        tg_style buf;
        tg_style_manner(&buf, s, 0);               /* (the profile: manner 0 changes nothing) */
        printf("%d %s\n", s, buf.name);
        for (int k = 0; k < NF; ++k) {
            const tg_field *f = field(k);
            int n = ksize(f->kind) * f->count;
            printf(" %d:", k);
            for (int i = 0; i < n; ++i)
                printf("%02x", ((const uint8_t *)&buf)[f->off + i]);
        }
        printf("\n");
    }
}

#if !TUNA_PACKED_STYLES
static uint8_t blob[200000];
static int nblob;

/* the bytes of field k element i of style st */
static const uint8_t *elem(const tg_style *st, int k, int i)
{
    const tg_field *f = field(k);
    return (const uint8_t *)st + f->off + i * ksize(f->kind);
}

static void emit(const tg_style *st, const tg_style *base)
{
    for (int k = 0; k < NF; ++k) {
        const tg_field *f = field(k);
        int sz = ksize(f->kind);
        for (int i = 0; i < f->count; ++i) {
            if (base && !memcmp(elem(st, k, i), elem(base, k, i), (size_t)sz))
                continue;
            if (!base) {                           /* the base: what differs from zero */
                static const uint8_t z[4];
                if (!memcmp(elem(st, k, i), z, (size_t)sz))
                    continue;
            }
            blob[nblob++] = (uint8_t)k;
            blob[nblob++] = (uint8_t)i;
            memcpy(blob + nblob, elem(st, k, i), (size_t)sz);
            nblob += sz;
        }
    }
    blob[nblob++] = 0xFF;
}

/* every byte of a profile is in a field, but for the name and the padding (which the table leaves zero) */
static void check_cover(void)
{
    static uint8_t cov[sizeof(tg_style)];
    memset(cov, 0, sizeof cov);
    for (int k = 0; k < NF; ++k) {
        const tg_field *f = field(k);
        memset(cov + f->off, 1, (size_t)(ksize(f->kind) * f->count));
    }
    memset(cov + offsetof(tg_style, name), 1, sizeof(const char *));
    for (int s = 0; s < TG_STYLE_COUNT; ++s)
        for (size_t b = 0; b < sizeof(tg_style); ++b)
            if (!cov[b] && ((const uint8_t *)&styles[s])[b]) {
                fprintf(stderr, "tuna_style_pack: %s: byte %zu is in no field\n", styles[s].name, b);
                exit(1);
            }
}

static int write_inc(FILE *o)
{
    /* the base: each element's commonest value among the styles */
    static tg_style base;
    memset(&base, 0, sizeof base);
    for (int k = 0; k < NF; ++k) {
        const tg_field *f = field(k);
        int sz = ksize(f->kind);
        for (int i = 0; i < f->count; ++i) {
            int best = 0, bestn = -1;
            for (int a = 0; a < TG_STYLE_COUNT; ++a) {
                int n = 0;
                for (int b = 0; b < TG_STYLE_COUNT; ++b)
                    n += !memcmp(elem(&styles[a], k, i), elem(&styles[b], k, i), (size_t)sz);
                if (n > bestn)
                    bestn = n, best = a;
            }
            memcpy((uint8_t *)&base + f->off + i * sz, elem(&styles[best], k, i), (size_t)sz);
        }
    }
    int off[TG_STYLE_COUNT];
    emit(&base, 0);
    int nbase = nblob;
    for (int s = 0; s < TG_STYLE_COUNT; ++s) {
        off[s] = nblob;
        emit(&styles[s], &base);
    }
    fprintf(o, "/* SPDX-License-Identifier: GPL-3.0-only */\n"
               "/* generated by tests/tuna_style_pack.c from tuna_style.c's table: the %d profiles as their differences\n"
               " * from a base (%d bytes in all, the table %d); tuna_style.c expands them (TUNA_PACKED_STYLES). Edit\n"
               " * tuna_style.c, then: build/host/tuna_style_pack firmware/src/tuna_styles_packed.inc (tests/tuna_tests.sh checks it) */\n",
            TG_STYLE_COUNT, nblob, (int)(TG_STYLE_COUNT * sizeof(tg_style)));
    fprintf(o, "static const char *const TGP_NAME[%d] = {", TG_STYLE_COUNT);
    for (int s = 0; s < TG_STYLE_COUNT; ++s)
        fprintf(o, "%s\"%s\",", s % 8 ? " " : "\n    ", styles[s].name);
    fprintf(o, "\n};\nstatic const uint%d_t TGP_OFF[%d] = {", nblob < 65536 ? 16 : 32, TG_STYLE_COUNT);
    for (int s = 0; s < TG_STYLE_COUNT; ++s)
        fprintf(o, "%s%d,", s % 12 ? " " : "\n    ", off[s]);
    fprintf(o, "\n};\n#define TGP_BASE_LEN %d\nstatic const uint8_t TGP_BLOB[%d] = {", nbase, nblob);
    for (int i = 0; i < nblob; ++i)
        fprintf(o, "%s%d,", i % 24 ? "" : "\n    ", blob[i]);
    fprintf(o, "\n};\n");
    return nblob;
}
#endif

int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "--print")) {
        print_all();
        return 0;
    }
#if TUNA_PACKED_STYLES
    fprintf(stderr, "tuna_style_pack: built packed: --print only\n");
    return 2;
#else
    int check = argc > 2 && !strcmp(argv[1], "--check");
    const char *path = argv[argc - 1];
    if (argc < 2) {
        fprintf(stderr, "usage: tuna_style_pack [--check] OUT.inc | --print\n");
        return 2;
    }
    check_cover();
    char *text = 0;
    size_t len = 0;
    FILE *m = open_memstream(&text, &len);
    int n = write_inc(m);
    fclose(m);
    if (check) {
        FILE *f = fopen(path, "rb");
        char *old = malloc(len + 2);
        size_t got = f ? fread(old, 1, len + 1, f) : 0;
        if (f)
            fclose(f);
        if (got != len || memcmp(old, text, len)) {
            fprintf(stderr, "tuna_style_pack: %s is out of date: build/host/tuna_style_pack %s writes it\n", path, path);
            return 1;
        }
        printf("tuna_style_pack: %d styles in %d bytes, up to date\n", TG_STYLE_COUNT, n);
        return 0;
    }
    FILE *f = fopen(path, "wb");
    if (!f || fwrite(text, 1, len, f) != len)
        return 1;
    fclose(f);
    printf("tuna_style_pack: %d styles in %d bytes (the table: %d) -> %s\n", TG_STYLE_COUNT, n,
           (int)(TG_STYLE_COUNT * sizeof(tg_style)), path);
    return 0;
#endif
}
