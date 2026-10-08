/* SPDX-License-Identifier: GPL-3.0-only */
/* Every style's profile as JSON, field by field (tools/tuna_manners.py: the base the manners change). */
#include <stddef.h>
#include <stdio.h>
#define TG_FIELD_NAMES
#include "tuna_style.h"
#include "tuna_style_fields.inc"

static void value(const tg_style *st, const tg_field *f, int i)
{
    const uint8_t *p = (const uint8_t *)st + f->off;
    switch (f->kind) {
    case TGK_F32: printf("%g", (double)((const float *)p)[i]); break;
    case TGK_I32: printf("%d", ((const int *)p)[i]); break;
    case TGK_U8: printf("%u", p[i]); break;
    case TGK_I8: printf("%d", ((const int8_t *)p)[i]); break;
    case TGK_BOOL: printf("%d", ((const bool *)p)[i] ? 1 : 0); break;
    case TGK_U16: printf("%u", ((const uint16_t *)p)[i]); break;
    case TGK_I16: printf("%d", ((const int16_t *)p)[i]); break;
    case TGK_U32: printf("%u", ((const uint32_t *)p)[i]); break;
    }
}

int main(void)
{
    printf("[\n");
    for (int s = 0; s < TG_STYLE_COUNT; ++s) {
        const tg_style *st = tg_style_get(s);
        printf(" {\"name\": \"%s\"", st->name);
        for (int k = 0; k < TGF_COUNT; ++k) {
            const tg_field *f = &TG_FIELDS[k];
            printf(", \"%s\": ", TG_FIELD_NAME[k]);
            if (f->count > 1) printf("[");
            for (int i = 0; i < f->count; ++i) {
                if (i) printf(", ");
                value(st, f, i);
            }
            if (f->count > 1) printf("]");
        }
        printf("}%s\n", s + 1 < TG_STYLE_COUNT ? "," : "");
    }
    printf("]\n");
    return 0;
}
