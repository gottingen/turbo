
#ifndef  TURBO_VARIABLE_DETAIL_CALL_OP_RETURNING_VOID_H_
#define  TURBO_VARIABLE_DETAIL_CALL_OP_RETURNING_VOID_H_

namespace turbo {
    namespace metrics_detail {

        template<typename Op, typename T1, typename T2>
        inline void call_op_returning_void(
                const Op &op, T1 &v1, const T2 &v2) {
            return op(v1, v2);
        }

    }  // namespace metrics_detail
}  // namespace turbo

#endif  // TURBO_VARIABLE_DETAIL_CALL_OP_RETURNING_VOID_H_
