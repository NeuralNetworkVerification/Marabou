#ifndef EFFORT_H
#define EFFORT_H

namespace prop {
/**
 * Subclasses of Theory may add additional efforts.  DO NOT CHECK
 * equality with one of these values (e.g. if STANDARD xxx) but
 * rather use range checks (or use the helper functions below).
 * Normally we call QUICK_CHECK or STANDARD; at the leaves we call
 * with FULL_EFFORT.
 */
enum Effort {
    /**
     * Standard effort where theory need not do anything
     */
    EFFORT_STANDARD = 50,
    /**
     * Full effort requires the theory make sure its assertions are
     * satisfiable or not
     */
    EFFORT_FULL = 100,
    /**
     * Last call effort, called after theory combination has completed with
     * no lemmas and a model is available.
     */
    EFFORT_LAST_CALL = 200
}; /* enum Effort */
} // namespace prop

#endif
