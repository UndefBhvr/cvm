#include "../include/icm_sparse_api.hpp"

namespace icm {

// Create/destroy context
Context* create_context() {
    return new Context();
}

void destroy_context(Context* ctx) {
    delete ctx;
}

} // namespace icm