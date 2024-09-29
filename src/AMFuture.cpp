//
// Created by zdenek on 18.8.22.
//

#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
#else
#include "../AMFuture.h"
#include <set>

namespace AMCore {

    std::set<_AMFutureZombieBase *> _AMFutureZombieBase::m_zombies;

    bool _AMFutureZombieBase::checkZombies() {
        if (m_zombies.size() > 0) {
            for (std::set<_AMFutureZombieBase *>::iterator it = m_zombies.begin(); it != m_zombies.end();) {
                if ((*it)->valid()) {
                    if ((*it)->ready()) {
                        (*it)->get();
                        _AMFutureZombieBase *p = *it;
                        it = m_zombies.erase(it);
                        delete p;
                        //printf("Mazu AMFuture zombie, zbývá %li\n", m_zombies.size());
                        if (it == m_zombies.end()) {
                            break;
                        }
                    }
                } else {
                    _AMFutureZombieBase *p = (*it);
                    it = m_zombies.erase(it);
                    delete p;
                    //printf("Mazu neval zombie, zbývá %li\n", m_zombies.size());
                    if (it == m_zombies.end()) {
                        break;
                    }
                }
            }
            return m_zombies.size() == 0;
        }
        return true;
    }

    void _AMFutureZombieBase::add(_AMFutureZombieBase *p) {
        m_zombies.insert(p);
    };

    _AMFutureZombieBase::~_AMFutureZombieBase() {

    }

}
#endif