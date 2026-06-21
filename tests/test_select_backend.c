#include "../loadbalancer.h"
#include <assert.h>
int main() {
    struct load_balancer lb = {0};
    init_loadbalancer(&lb);
    for (int i = 0; i < 6; i++) {
        struct backend * backend = select_backend(&lb);
        assert(backend->port == 9001 + (i % 3));
    }
    assert(lb.current_backend == 0);
    return 0;
}