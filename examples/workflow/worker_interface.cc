// This program demonstrates how to change the worker behavior
// upon the creation of an executor.

#include <turbo/strings/str_cat.h>
#include <turbo/workflow/workflow.h>

class CustomWorkerBehavior : public turbo::WorkerInterface {

  public:
  
  // to call before the worker enters the scheduling loop
  void scheduler_prologue(turbo::Worker& w) override {
    std::cout <<
      "worker "<< w.id()<< " (native="<< w.thread()->native_handle()<< ") enters scheduler\n"
    ;
  }

  // to call after the worker leaves the scheduling loop
  void scheduler_epilogue(turbo::Worker& w, std::exception_ptr) override {
    std::cout <<"worker "<< w.id()<< " (native="<< w.thread()->native_handle()<< ") leaves scheduler\n";
  }
};

int main() {

  turbo::Executor executor(4, std::make_shared<CustomWorkerBehavior>());
  
  return 0;
}
