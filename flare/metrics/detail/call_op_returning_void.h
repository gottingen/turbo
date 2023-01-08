
#ifndef  FLARE_VARIABLE_DETAIL_CALL_OP_RETURNING_VOID_H_
#define  FLARE_VARIABLE_DETAIL_CALL_OP_RETURNING_VOID_H_

namespace flare {
    namespace metrics_detail {

        template<typename Op, typename T1, typename T2>
        inline void call_op_returning_void(
                const Op &op, T1 &v1, const T2 &v2) {
            return op(v1, v2);
        }

    }  // namespace metrics_detail
}  // namespace flare

#endif  // FLARE_VARIABLE_DETAIL_CALL_OP_RETURNING_VOID_H_
