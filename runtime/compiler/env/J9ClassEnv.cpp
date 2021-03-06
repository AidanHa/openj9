/*******************************************************************************
 * Copyright (c) 2000, 2019 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "compile/Compilation.hpp"
#if defined(JITSERVER_SUPPORT)
#include "control/CompilationRuntime.hpp"
#include "control/CompilationThread.hpp"
#include "control/JITServerHelpers.hpp"
#endif /* defined(JITSERVER_SUPPORT) */
#include "env/ClassEnv.hpp"
#include "env/CompilerEnv.hpp"
#include "env/jittypes.h"
#include "env/VMJ9.h"
#include "j9.h"
#include "j9protos.h"
#include "j9cp.h"
#include "j9cfg.h"
#include "rommeth.h"


/*  REQUIRES STATE (_vmThread).  MOVE vmThread to COMPILATION

TR_OpaqueClassBlock *
J9::ClassEnv::getClassFromJavaLangClass(uintptrj_t objectPointer)
   {
   return (TR_OpaqueClassBlock*)J9VM_J9CLASS_FROM_HEAPCLASS(_vmThread, objectPointer);
   }
*/

TR::ClassEnv *
J9::ClassEnv::self()
   {
   return static_cast<TR::ClassEnv *>(this);
   }

J9Class *
J9::ClassEnv::convertClassOffsetToClassPtr(TR_OpaqueClassBlock *clazzOffset)
   {
   // NOTE : We could pass down vmThread() in the call below if the conversion
   // required the VM thread. Currently it does not. If we did change that
   // such that the VM thread was reqd, we would need to handle AOT where the
   // TR_FrontEnd is created with a NULL J9VMThread object.
   //
   return (J9Class*)((TR_OpaqueClassBlock *)clazzOffset);
   }

TR_OpaqueClassBlock *
J9::ClassEnv::convertClassPtrToClassOffset(J9Class *clazzPtr)
   {
   // NOTE : We could pass down vmThread() in the call below if the conversion
   // required the VM thread. Currently it does not. If we did change that
   // such that the VM thread was reqd, we would need to handle AOT where the
   // TR_FrontEnd is created with a NULL J9VMThread object.
   //
   return (TR_OpaqueClassBlock*)(clazzPtr);
   }

bool
J9::ClassEnv::isClassSpecialForStackAllocation(TR_OpaqueClassBlock * clazz)
   {
   const UDATA mask = (J9AccClassReferenceWeak |
                       J9AccClassReferenceSoft |
                       J9AccClassFinalizeNeeded |
                       J9AccClassOwnableSynchronizer);

#if defined(JITSERVER_SUPPORT)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      uintptrj_t classDepthAndFlags = 0;
      JITServerHelpers::getAndCacheRAMClassInfo((J9Class *)clazz, TR::compInfoPT->getClientData(), stream, JITServerHelpers::CLASSINFO_CLASS_DEPTH_AND_FLAGS, (void *)&classDepthAndFlags);
      return ((classDepthAndFlags & mask)?true:false);
      }
   else
#endif /* defined(JITSERVER_SUPPORT) */
      {
      if (((J9Class *)clazz)->classDepthAndFlags & mask)
         {
         return true;
         }
      }

   return false;
   }

uintptrj_t
J9::ClassEnv::classFlagsValue(TR_OpaqueClassBlock * classPointer)
   {
#if defined(JITSERVER_SUPPORT)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      stream->write(JITServer::MessageType::ClassEnv_classFlagsValue, classPointer);
      return std::get<0>(stream->read<uintptrj_t>());
      }
#endif /* defined(JITSERVER_SUPPORT) */
   return (TR::Compiler->cls.convertClassOffsetToClassPtr(classPointer)->classFlags);
   }

uintptrj_t
J9::ClassEnv::classFlagReservableWordInitValue(TR_OpaqueClassBlock * classPointer)
   {
#if defined(JITSERVER_SUPPORT)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      uintptrj_t classFlags = 0;
      JITServerHelpers::getAndCacheRAMClassInfo((J9Class *)classPointer, TR::compInfoPT->getClientData(), stream, JITServerHelpers::CLASSINFO_CLASS_FLAGS, (void *)&classFlags);
#ifdef DEBUG
      stream->write(JITServer::MessageType::ClassEnv_classFlagsValue, classPointer);
      uintptrj_t classFlagsRemote = std::get<0>(stream->read<uintptrj_t>());
      // Check that class flags from remote call is equal to the cached ones
      classFlags = classFlags & J9ClassReservableLockWordInit;
      classFlagsRemote = classFlagsRemote & J9ClassReservableLockWordInit;
      TR_ASSERT(classFlags == classFlagsRemote, "remote call class flags is not equal to cached class flags");
#endif
      return classFlags & J9ClassReservableLockWordInit;
      }
#endif /* defined(JITSERVER_SUPPORT) */
   return (TR::Compiler->cls.convertClassOffsetToClassPtr(classPointer)->classFlags) & J9ClassReservableLockWordInit;
   }

uintptrj_t
J9::ClassEnv::classDepthOf(TR_OpaqueClassBlock * clazzPointer)
   {
#if defined(JITSERVER_SUPPORT)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      uintptrj_t classDepthAndFlags = 0;
      JITServerHelpers::getAndCacheRAMClassInfo((J9Class *)clazzPointer, TR::compInfoPT->getClientData(), stream, JITServerHelpers::CLASSINFO_CLASS_DEPTH_AND_FLAGS, (void *)&classDepthAndFlags);
      return (classDepthAndFlags & J9AccClassDepthMask);
      }
#endif /* defined(JITSERVER_SUPPORT) */
   return J9CLASS_DEPTH(TR::Compiler->cls.convertClassOffsetToClassPtr(clazzPointer));
   }


uintptrj_t
J9::ClassEnv::classInstanceSize(TR_OpaqueClassBlock * clazzPointer)
   {
#if defined(JITSERVER_SUPPORT)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      uintptrj_t totalInstanceSize = 0;
      JITServerHelpers::getAndCacheRAMClassInfo((J9Class *)clazzPointer, TR::compInfoPT->getClientData(), stream, JITServerHelpers::CLASSINFO_TOTAL_INSTANCE_SIZE, (void *)&totalInstanceSize);
      return totalInstanceSize;
      }
#endif /* defined(JITSERVER_SUPPORT) */
   return TR::Compiler->cls.convertClassOffsetToClassPtr(clazzPointer)->totalInstanceSize;
   }


J9ROMClass *
J9::ClassEnv::romClassOf(TR_OpaqueClassBlock * clazz)
   {
   J9Class *j9clazz = TR::Compiler->cls.convertClassOffsetToClassPtr(clazz);
#if defined(JITSERVER_SUPPORT)
   if (TR::compInfoPT && TR::compInfoPT->getStream())
      {
      return TR::compInfoPT->getAndCacheRemoteROMClass(j9clazz);
      }
#endif /* defined(JITSERVER_SUPPORT) */
   return j9clazz->romClass;
   }

J9Class **
J9::ClassEnv::superClassesOf(TR_OpaqueClassBlock * clazz)
   {
#if defined(JITSERVER_SUPPORT)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      stream->write(JITServer::MessageType::ClassEnv_superClassesOf, clazz);
      return std::get<0>(stream->read<J9Class **>());
      }
#endif /* defined(JITSERVER_SUPPORT) */
   return TR::Compiler->cls.convertClassOffsetToClassPtr(clazz)->superclasses;
   }

J9ROMClass *
J9::ClassEnv::romClassOfSuperClass(TR_OpaqueClassBlock * clazz, size_t index)
   {
#if defined(JITSERVER_SUPPORT)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      stream->write(JITServer::MessageType::ClassEnv_indexedSuperClassOf, clazz, index);
      J9Class *j9clazz = std::get<0>(stream->read<J9Class *>());
      return TR::compInfoPT->getAndCacheRemoteROMClass(j9clazz);
      }
#endif /* defined(JITSERVER_SUPPORT) */
   return self()->superClassesOf(clazz)[index]->romClass;
   }

J9ITable *
J9::ClassEnv::iTableOf(TR_OpaqueClassBlock * clazz)
   {
#if defined(JITSERVER_SUPPORT)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      stream->write(JITServer::MessageType::ClassEnv_iTableOf, clazz);
      return std::get<0>(stream->read<J9ITable*>());
      }
#endif /* defined(JITSERVER_SUPPORT) */
   return (J9ITable*) self()->convertClassOffsetToClassPtr(clazz)->iTable;
   }

J9ITable *
J9::ClassEnv::iTableNext(J9ITable *current)
   {
#if defined(JITSERVER_SUPPORT)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      stream->write(JITServer::MessageType::ClassEnv_iTableNext, current);
      return std::get<0>(stream->read<J9ITable*>());
      }
#endif /* defined(JITSERVER_SUPPORT) */
   return current->next;
   }

J9ROMClass *
J9::ClassEnv::iTableRomClass(J9ITable *current)
   {
#if defined(JITSERVER_SUPPORT)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      stream->write(JITServer::MessageType::ClassEnv_iTableRomClass, current);
      return std::get<0>(stream->read<J9ROMClass*>());
      }
#endif /* defined(JITSERVER_SUPPORT) */
   return current->interfaceClass->romClass;
   }

#if defined(JITSERVER_SUPPORT)
std::vector<TR_OpaqueClassBlock *>
J9::ClassEnv::getITable(TR_OpaqueClassBlock *clazz)
   {
   if (auto stream = TR::CompilationInfo::getStream())
      {
      // This normally shouldn't be called from the server,
      // because it will have a cached table
      stream->write(JITServer::MessageType::ClassEnv_getITable, clazz);
      return std::get<0>(stream->read<std::vector<TR_OpaqueClassBlock *>>());
      }
   std::vector<TR_OpaqueClassBlock *> iTableSerialization;
   iTableSerialization.reserve((TR::Compiler->cls.romClassOf(clazz)->interfaceCount));
   for (J9ITable *iTableCur = TR::Compiler->cls.iTableOf(clazz); iTableCur; iTableCur = iTableCur->next)
      iTableSerialization.push_back((TR_OpaqueClassBlock *) iTableCur->interfaceClass);
   return iTableSerialization;
   }
#endif /* defined(JITSERVER_SUPPORT) */

bool
J9::ClassEnv::isStringClass(TR_OpaqueClassBlock *clazz)
   {
   //return (J9Class*)clazz == J9VMJAVALANGSTRING(jitConfig->javaVM);
   return false;
   }


bool
J9::ClassEnv::isStringClass(uintptrj_t objectPointer)
   {
   /*
   TR_ASSERT(TR::Compiler->vm.hasAccess(omrVMThread), "isString requires VM access");
   return isString(getObjectClass(objectPointer));
   */
   return false;
   }

bool
J9::ClassEnv::isAbstractClass(TR::Compilation *comp, TR_OpaqueClassBlock *clazzPointer)
   {
   return comp->fej9()->isAbstractClass(clazzPointer);
   }

bool
J9::ClassEnv::isInterfaceClass(TR::Compilation *comp, TR_OpaqueClassBlock *clazzPointer)
   {
   return comp->fej9()->isInterfaceClass(clazzPointer);
   }

bool
J9::ClassEnv::isEnumClass(TR::Compilation *comp, TR_OpaqueClassBlock *clazzPointer, TR_ResolvedMethod *method)
   {
   return comp->fej9()->isEnumClass(clazzPointer, method);
   }

bool
J9::ClassEnv::isPrimitiveClass(TR::Compilation *comp, TR_OpaqueClassBlock *clazz)
   {
   return comp->fej9()->isPrimitiveClass(clazz);
   }

bool
J9::ClassEnv::isPrimitiveArray(TR::Compilation *comp, TR_OpaqueClassBlock *clazz)
   {
   return comp->fej9()->isPrimitiveArray(clazz);
   }

bool
J9::ClassEnv::isReferenceArray(TR::Compilation *comp, TR_OpaqueClassBlock *clazz)
   {
   return comp->fej9()->isReferenceArray(clazz);
   }

bool
J9::ClassEnv::isClassArray(TR::Compilation *comp, TR_OpaqueClassBlock *clazz)
   {
   return comp->fej9()->isClassArray(clazz);
   }

bool
J9::ClassEnv::isClassFinal(TR::Compilation *comp, TR_OpaqueClassBlock *clazz)
   {
   return comp->fej9()->isClassFinal(clazz);
   }

bool
J9::ClassEnv::hasFinalizer(TR::Compilation *comp, TR_OpaqueClassBlock *clazz)
   {
   return comp->fej9()->hasFinalizer(clazz);
   }

bool
J9::ClassEnv::isClassInitialized(TR::Compilation *comp, TR_OpaqueClassBlock *clazz)
   {
   return comp->fej9()->isClassInitialized(clazz);
   }

bool
J9::ClassEnv::classHasIllegalStaticFinalFieldModification(TR_OpaqueClassBlock * clazzPointer)
   {
#if defined(JITSERVER_SUPPORT)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      stream->write(JITServer::MessageType::ClassEnv_classHasIllegalStaticFinalFieldModification, clazzPointer);
      return std::get<0>(stream->read<bool>());
      }
#endif /* defined(JITSERVER_SUPPORT) */
   J9Class* j9clazz = TR::Compiler->cls.convertClassOffsetToClassPtr(clazzPointer);
   return J9_ARE_ANY_BITS_SET(j9clazz->classFlags, J9ClassHasIllegalFinalFieldModifications);
   }

bool
J9::ClassEnv::hasFinalFieldsInClass(TR::Compilation *comp, TR_OpaqueClassBlock *clazz)
   {
   return comp->fej9()->hasFinalFieldsInClass(clazz);
   }

bool
J9::ClassEnv::sameClassLoaders(TR::Compilation *comp, TR_OpaqueClassBlock *class1, TR_OpaqueClassBlock *class2)
   {
   return comp->fej9()->sameClassLoaders(class1, class2);
   }

bool
J9::ClassEnv::isString(TR::Compilation *comp, TR_OpaqueClassBlock *clazz)
   {
   return comp->fej9()->isString(clazz);
   }

bool
J9::ClassEnv::isString(TR::Compilation *comp, uintptrj_t objectPointer)
   {
   return comp->fej9()->isString(objectPointer);
   }

bool
J9::ClassEnv::jitStaticsAreSame(
      TR::Compilation *comp,
      TR_ResolvedMethod * method1,
      int32_t cpIndex1,
      TR_ResolvedMethod * method2,
      int32_t cpIndex2)
   {
   return comp->fej9()->jitStaticsAreSame(method1, cpIndex1, method2, cpIndex2);
   }

bool
J9::ClassEnv::jitFieldsAreSame(
      TR::Compilation *comp,
      TR_ResolvedMethod * method1,
      int32_t cpIndex1,
      TR_ResolvedMethod * method2,
      int32_t cpIndex2,
      int32_t isStatic)
   {
   return comp->fej9()->jitFieldsAreSame(method1, cpIndex1, method2, cpIndex2, isStatic);
   }

bool
J9::ClassEnv::isAnonymousClass(TR::Compilation *comp, TR_OpaqueClassBlock *clazz)
   {
   return comp->fej9()->isAnonymousClass(clazz);
   }


int32_t
J9::ClassEnv::vTableSlot(TR::Compilation *comp, TR_OpaqueMethodBlock *method, TR_OpaqueClassBlock *clazz)
   {
   return comp->fej9()->getVTableSlot(method, clazz);
   }


int32_t
J9::ClassEnv::flagValueForPrimitiveTypeCheck(TR::Compilation *comp)
   {
   return comp->fej9()->getFlagValueForPrimitiveTypeCheck();
   }


int32_t
J9::ClassEnv::flagValueForArrayCheck(TR::Compilation *comp)
   {
   return comp->fej9()->getFlagValueForArrayCheck();
   }


int32_t
J9::ClassEnv::flagValueForFinalizerCheck(TR::Compilation *comp)
   {
   return comp->fej9()->getFlagValueForFinalizerCheck();
   }


// this should be a method of TR_SymbolReference
char *
J9::ClassEnv::classNameChars(TR::Compilation *comp, TR::SymbolReference * symRef, int32_t & length)
   {
   return comp->fej9()->classNameChars(comp, symRef, length);
   }


char *
J9::ClassEnv::classNameChars(TR::Compilation *comp, TR_OpaqueClassBlock *clazz, int32_t & length)
   {
   return comp->fej9()->getClassNameChars(clazz, length);
   }


char *
J9::ClassEnv::classSignature_DEPRECATED(TR::Compilation *comp, TR_OpaqueClassBlock * clazz, int32_t & length, TR_Memory *m)
   {
   return comp->fej9()->getClassSignature_DEPRECATED(clazz, length, m);
   }


char *
J9::ClassEnv::classSignature(TR::Compilation *comp, TR_OpaqueClassBlock * clazz, TR_Memory *m)
   {
   return comp->fej9()->getClassSignature(clazz, m);
   }

uintptrj_t
J9::ClassEnv::persistentClassPointerFromClassPointer(TR::Compilation *comp, TR_OpaqueClassBlock *clazz)
   {
   return comp->fej9()->getPersistentClassPointerFromClassPointer(clazz);
   }

TR_OpaqueClassBlock *
J9::ClassEnv::objectClass(TR::Compilation *comp, uintptrj_t objectPointer)
   {
   return comp->fej9()->getObjectClass(objectPointer);
   }

TR_OpaqueClassBlock *
J9::ClassEnv::classFromJavaLangClass(TR::Compilation *comp, uintptrj_t objectPointer)
   {
   return comp->fej9()->getClassFromJavaLangClass(objectPointer);
   }


uint16_t
J9::ClassEnv::getStringCharacter(TR::Compilation *comp, uintptrj_t objectPointer, int32_t index)
   {
   return comp->fej9()->getStringCharacter(objectPointer, index);
   }


bool
J9::ClassEnv::getStringFieldByName(TR::Compilation *comp, TR::SymbolReference *stringRef, TR::SymbolReference *fieldRef, void* &pResult)
   {
   return comp->fej9()->getStringFieldByName(comp, stringRef, fieldRef, pResult);
   }

uintptrj_t
J9::ClassEnv::getArrayElementWidthInBytes(TR::Compilation *comp, TR_OpaqueClassBlock* arrayClass)
   {
   TR_ASSERT(TR::Compiler->cls.isClassArray(comp, arrayClass), "Class must be array");
   int32_t logElementSize = ((J9ROMArrayClass*)((J9Class*)arrayClass)->romClass)->arrayShape & 0x0000FFFF;
   return 1 << logElementSize;
   }

intptrj_t
J9::ClassEnv::getVFTEntry(TR::Compilation *comp, TR_OpaqueClassBlock* clazz, int32_t offset)
   {
   return comp->fej9()->getVFTEntry(clazz, offset);
   }

uint8_t *
J9::ClassEnv::getROMClassRefName(TR::Compilation *comp, TR_OpaqueClassBlock *clazz, uint32_t cpIndex, int &classRefLen)
   {
   J9ROMConstantPoolItem *romCP = self()->getROMConstantPool(comp, clazz);
   J9ROMFieldRef *romFieldRef = (J9ROMFieldRef *)&romCP[cpIndex];
   J9ROMClassRef *romClassRef = (J9ROMClassRef *)&romCP[romFieldRef->classRefCPIndex];
#if defined(JITSERVER_SUPPORT)
   if (comp->isOutOfProcessCompilation())
      {
      TR::CompilationInfoPerThread *compInfoPT = TR::compInfoPT;
      char *name;
      OMR::CriticalSection getRemoteROMClass(compInfoPT->getClientData()->getROMMapMonitor()); 
      auto &classMap = compInfoPT->getClientData()->getROMClassMap();
      auto it = classMap.find(reinterpret_cast<J9Class *>(clazz));
      auto &classInfo = it->second;
      name = classInfo.getROMString(classRefLen, romClassRef,
                             {
                             offsetof(J9ROMClassRef, name)
                             });
      return (uint8_t *) name;
      }
#endif /* defined(JITSERVER_SUPPORT) */
   J9UTF8 *classRefNameUtf8 = J9ROMCLASSREF_NAME(romClassRef);
   classRefLen = J9UTF8_LENGTH(classRefNameUtf8);
   uint8_t *classRefName = J9UTF8_DATA(classRefNameUtf8);
   return classRefName;
   }

J9ROMConstantPoolItem *
J9::ClassEnv::getROMConstantPool(TR::Compilation *comp, TR_OpaqueClassBlock *clazz)
   {
#if defined(JITSERVER_SUPPORT)
   if (comp->isOutOfProcessCompilation())
      {
      J9ROMClass *romClass = TR::compInfoPT->getAndCacheRemoteROMClass(reinterpret_cast<J9Class *>(clazz));
      return (J9ROMConstantPoolItem *) ((UDATA) romClass + sizeof(J9ROMClass));
      }
#endif /* defined(JITSERVER_SUPPORT) */
   J9ConstantPool *ramCP = reinterpret_cast<J9ConstantPool *>(comp->fej9()->getConstantPoolFromClass(clazz));
   return ramCP->romConstantPool;
   }
