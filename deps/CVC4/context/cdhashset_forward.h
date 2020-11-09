/*********************                                                        */
/*! \file cdhashset_forward.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Morgan Deters, Tim King, Dejan Jovanovic
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief This is a forward declaration header to declare the CDSet<>
 ** template
 **
 ** This is a forward declaration header to declare the CDSet<>
 ** template.  It's useful if you want to forward-declare CDSet<>
 ** without including the full cdset.h header, for example, in a
 ** public header context.
 **
 ** For CDSet<> in particular, it's difficult to forward-declare it
 ** yourself, because it has a default template argument.
 **/

#include "cvc4_public.h"

#ifndef CVC4__CONTEXT__CDSET_FORWARD_H
#define CVC4__CONTEXT__CDSET_FORWARD_H

#include <functional>

namespace CVC4 {
namespace context {
template <class V, class HashFcn = std::hash<V> >
class CDHashSet;
}  // namespace context
}  // namespace CVC4

#endif /* CVC4__CONTEXT__CDSET_FORWARD_H */
