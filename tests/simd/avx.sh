#!/bin/sh
exit_code=0
for instr in `grep -o -E '^_mm256_[a-z1Z0-9_]+' $0`
do
    if ! grep -q -r $instr ../include-refactoring
    then
        echo $instr
        exit_code=1
    fi
done
exit $exit_code

# Instructions below starting with a # are known to be unused in simd
_mm256_add_pd
_mm256_add_ps
#_mm256_addsub_pd
#_mm256_addsub_ps
_mm256_and_pd
_mm256_and_ps
_mm256_andnot_pd
_mm256_andnot_ps
_mm256_blend_pd
_mm256_blend_ps
_mm256_blendv_pd
_mm256_blendv_ps
#_mm256_broadcast_pd
#_mm256_broadcast_ps
#_mm256_broadcast_sd
#_mm256_broadcast_ss
_mm256_castpd_ps
_mm256_castpd_si256
#_mm256_castpd128_pd256
#_mm256_castpd256_pd128
_mm256_castps_pd
_mm256_castps_si256
#_mm256_castps128_ps256
#_mm256_castps256_ps128
_mm256_castsi128_si256
_mm256_castsi256_pd
_mm256_castsi256_ps
_mm256_castsi256_si128
_mm256_ceil_pd
_mm256_ceil_ps
_mm256_cmp_pd
_mm256_cmp_ps
#_mm256_cvtepi32_pd
_mm256_cvtepi32_ps
#_mm256_cvtpd_epi32
#_mm256_cvtpd_ps
#_mm256_cvtps_epi32
#_mm256_cvtps_pd
#_mm256_cvtsd_f64
#_mm256_cvtsi256_si32
#_mm256_cvtss_f32
#_mm256_cvttpd_epi32
_mm256_cvttps_epi32
_mm256_div_pd
_mm256_div_ps
#_mm256_dp_ps
#_mm256_extract_epi32
#_mm256_extract_epi64
_mm256_extractf128_pd
_mm256_extractf128_ps
_mm256_extractf128_si256
_mm256_floor_pd
_mm256_floor_ps
_mm256_hadd_pd
_mm256_hadd_ps
#_mm256_hsub_pd
#_mm256_hsub_ps
#_mm256_insert_epi16
#_mm256_insert_epi32
#_mm256_insert_epi64
#_mm256_insert_epi8
_mm256_insertf128_pd
_mm256_insertf128_ps
_mm256_insertf128_si256
#_mm256_lddqu_si256
_mm256_load_pd
_mm256_load_ps
_mm256_load_si256
_mm256_loadu_pd
_mm256_loadu_ps
_mm256_loadu_si256
#_mm256_loadu2_m128
#_mm256_loadu2_m128d
#_mm256_loadu2_m128i
#_mm256_maskload_pd
#_mm256_maskload_ps
#_mm256_maskstore_pd
#_mm256_maskstore_ps
_mm256_max_pd
_mm256_max_ps
_mm256_min_pd
_mm256_min_ps
#_mm256_movedup_pd
#_mm256_movehdup_ps
#_mm256_moveldup_ps
#_mm256_movemask_pd
#_mm256_movemask_ps
_mm256_mul_pd
_mm256_mul_ps
_mm256_or_pd
_mm256_or_ps
#_mm256_permute_pd
#_mm256_permute_ps
_mm256_permute2f128_pd
_mm256_permute2f128_ps
#_mm256_permute2f128_si256
#_mm256_permutevar_pd
#_mm256_permutevar_ps
#_mm256_rcp_ps
_mm256_round_pd
_mm256_round_ps
#_mm256_rsqrt_ps
#_mm256_set_epi16
#_mm256_set_epi32
_mm256_set_epi64x
#_mm256_set_epi8
#_mm256_set_m128
#_mm256_set_m128d
#_mm256_set_m128i
#_mm256_set_pd
#_mm256_set_ps
_mm256_set1_epi16
_mm256_set1_epi32
_mm256_set1_epi64x
_mm256_set1_epi8
_mm256_set1_pd
_mm256_set1_ps
_mm256_setr_epi16
_mm256_setr_epi32
#_mm256_setr_epi64x
_mm256_setr_epi8
#_mm256_setr_m128
#_mm256_setr_m128d
#_mm256_setr_m128i
_mm256_setr_pd
_mm256_setr_ps
#_mm256_setzero_pd
#_mm256_setzero_ps
_mm256_setzero_si256
#_mm256_shuffle_pd
_mm256_shuffle_ps
_mm256_sqrt_pd
_mm256_sqrt_ps
_mm256_store_pd
_mm256_store_ps
_mm256_store_si256
_mm256_storeu_pd
_mm256_storeu_ps
_mm256_storeu_si256
#_mm256_storeu2_m128
#_mm256_storeu2_m128d
#_mm256_storeu2_m128i
#_mm256_stream_pd
#_mm256_stream_ps
#_mm256_stream_si256
_mm256_sub_pd
_mm256_sub_ps
_mm256_testc_pd
_mm256_testc_ps
_mm256_testc_si256
#_mm256_testnzc_pd
#_mm256_testnzc_ps
#_mm256_testnzc_si256
_mm256_testz_pd
_mm256_testz_ps
_mm256_testz_si256
#_mm256_undefined_pd
#_mm256_undefined_ps
#_mm256_undefined_si256
_mm256_unpackhi_pd
_mm256_unpackhi_ps
_mm256_unpacklo_pd
_mm256_unpacklo_ps
_mm256_xor_pd
_mm256_xor_ps
#_mm256_zeroall
#_mm256_zeroupper
#_mm256_zextpd128_pd256
#_mm256_zextps128_ps256
#_mm256_zextsi128_si256
