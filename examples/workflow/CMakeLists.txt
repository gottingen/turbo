

# turbo_cc_binary(
#   NAME
#     awesome_test
#   SRCS
#     "awesome_test.cc"
#   DEPS
#     turbo::awesome
# )

turbo_cc_binary (
    NAME
        async
    SRCS
        async.cc
    DEPS
        turbo::turbo
)

turbo_cc_binary (
    NAME
        attach_data
    SRCS
        attach_data.cc
    DEPS
        turbo::turbo
)

turbo_cc_binary (
    NAME
        cancel
    SRCS
        cancel.cc
    DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        cancel_async
        SRCS
        cancel_async.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        composition
        SRCS
        composition.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        condition
        SRCS
        condition.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        do_while_loop
        SRCS
        do_while_loop.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        fibonacci
        SRCS
        fibonacci.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        if_else
        SRCS
        if_else.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        limited_concurrency
        SRCS
        limited_concurrency.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        multi_condition
        SRCS
        multi_condition.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        nested_if_else
        SRCS
        nested_if_else.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        observer
        SRCS
        observer.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        parallel_data_pipeline
        SRCS
        parallel_data_pipeline.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        parallel_deferred_pipeline
        SRCS
        parallel_deferred_pipeline.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        parallel_for
        SRCS
        parallel_for.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        parallel_graph_pipeline
        SRCS
        parallel_graph_pipeline.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        parallel_sort
        SRCS
        parallel_sort.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        pipeline
        SRCS
        pipeline.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        priority
        SRCS
        priority.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        reduce
        SRCS
        reduce.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        run
        SRCS
        run.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        run_and_wait
        SRCS
        run_and_wait.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        runtime
        SRCS
        runtime.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        scalable_pipeline
        SRCS
        scalable_pipeline.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        simple
        SRCS
        simple.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        subflow
        SRCS
        subflow.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        subflow_async
        SRCS
        subflow_async.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        switch_case
        SRCS
        switch_case.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        workflow_pipeline
        SRCS
        workflow_pipeline.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        text_pipeline
        SRCS
        text_pipeline.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        visualization
        SRCS
        visualization.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        while_loop
        SRCS
        while_loop.cc
        DEPS
        turbo::turbo
)

turbo_cc_binary (
        NAME
        worker_interface
        SRCS
        worker_interface.cc
        DEPS
        turbo::turbo
)