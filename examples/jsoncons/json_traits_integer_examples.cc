// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include <cassert>
#include <string>
#include <vector>
#include <list>
#include <iomanip>
#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/csv/csv.h"
#include "turbo/jsoncons/bson/bson.h"
#include "turbo/jsoncons/cbor/cbor.h"
#include "turbo/jsoncons/msgpack/msgpack.h"
#include "turbo/jsoncons/ubjson/ubjson.h"

using namespace turbo;

namespace {

#if (defined(__GNUC__) || defined(__clang__)) && defined(TURBO_HAVE_INTRINSIC_INT128)
    void int128_example()
    {
        json j1("-18446744073709551617", semantic_tag::bigint);
        std::cout << j1 << "\n\n";

        __int128 val = j1.as<__int128>();

        json j2(val);

        assert(j2 == j1);
    }
#endif

} // namespace

void json_traits_integer_examples()
{
    std::cout << "\njson traits integer examples\n\n";

#if (defined(__GNUC__) || defined(__clang__)) && defined(TURBO_HAVE_INTRINSIC_INT128)
    int128_example();
#endif

    std::cout << std::endl;
}

