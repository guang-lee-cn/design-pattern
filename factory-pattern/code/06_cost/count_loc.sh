#!/bin/bash
cd /home/guang/code/design-pattern/factory-pattern/code || exit 1
for f in 01_birth/old.cpp 03_simple_factory/new.cpp 04_factory_method/factory_method.cpp \
         04_factory_method/registry.cpp 05_abstract_factory/abstract_factory.cpp \
         06_cost/variant_alt.cpp; do
    total=$(wc -l < "$f")
    effective=$(grep -vE '^[[:space:]]*(//|$)' "$f" | wc -l)
    printf '%-46s 总行 %-5s 有效行 %s\n' "$f" "$total" "$effective"
done
