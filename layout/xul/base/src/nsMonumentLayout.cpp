/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

//
// Eric Vaughan
// Netscape Communications
//
// See documentation in associated header file
//

#include "nsMonumentLayout.h"
#include "nsBoxLayoutState.h"
#include "nsIBox.h"
#include "nsBox.h"

//------ nsInfoListNodeImpl ----

void
nsBoxSizeListNodeImpl::Release(nsBoxLayoutState& aState)
{
   mRefCount--;
   if (mRefCount == 0) {
     Clear(aState);
     delete this;
   }
}

void
nsBoxSizeListNodeImpl::Desecrate()
{
  if (mParent)
    mParent->Desecrate();
}

void nsBoxSizeListNodeImpl::SetNext(nsBoxLayoutState& aState, nsBoxSizeList* aNext)
{
  if (mNext) {
    mNext->Release(aState);
  }

  mNext = aNext;
  aNext->AddRef();
}

void
nsBoxSizeListNodeImpl::Append(nsBoxLayoutState& aState, nsBoxSizeList* aChild)
{
  NS_ERROR("Attept at append to a leaf");
}

nsBoxSizeListNodeImpl::nsBoxSizeListNodeImpl(nsIBox* aBox):mNext(nsnull), 
                                                           mParent(nsnull), 
                                                           mRefCount(1),
                                                           mBox(aBox),
                                                           mIsSet(PR_FALSE)
{
}


nsBoxSizeList* 
nsBoxSizeListNodeImpl::GetAt(PRInt32 aIndex)
{
   nsBoxSizeList* node = this;
   PRInt32 count = 0;
   while(node)
   {
     if (count == aIndex)
        return node;

     node = node->GetNext();
     count++;
   }

   return nsnull;
}


//------ nsInfoListImpl2 ----

nsBoxSizeListImpl::nsBoxSizeListImpl(nsIBox* aBox):nsBoxSizeListNodeImpl(aBox),mFirst(nsnull),mLast(nsnull),mCount(0)
{
}

void
nsBoxSizeListImpl::Clear(nsBoxLayoutState& aState)
{
  nsBoxSizeList* list = mFirst;
  while(list)
  {
    list->Release(aState);
    list = list->GetNext();
  }

  mFirst = nsnull;
  mLast = nsnull;
}

void
nsBoxSizeListImpl::Append(nsBoxLayoutState& aState, nsBoxSizeList* aChild)
{
  if (!mFirst) 
      mFirst = aChild;
  else 
      mLast->SetNext(aState, aChild);

  mLast = aChild;
  aChild->SetParent(this);
  aChild->AddRef();
}

void
nsBoxSizeListImpl::Desecrate()
{
  if (mIsSet) {
    mIsSet = PR_FALSE;
    nsBoxSizeListNodeImpl::Desecrate();
  }
}

nsBoxSize
nsBoxSizeListImpl::GetBoxSize(nsBoxLayoutState& aState)
{
  if (!mIsSet) {

    mIsSet = PR_TRUE;

    mBoxSize.Clear();
    
    nsBoxSizeList* node = mFirst;

    PRBool isHorizontal = PR_FALSE;
    mBox->GetOrientation(isHorizontal);

    while(node) {
      nsIBox* box = node->GetBox();

      nsSize pref(0,0);
      nsSize min(0,0);
      nsSize max(NS_INTRINSICSIZE,NS_INTRINSICSIZE);
      nscoord ascent = 0;
      nscoord flex = 0;

      box->GetPrefSize(aState, pref);
      box->GetMinSize(aState, min);
      box->GetMaxSize(aState, max);
      box->GetAscent(aState, ascent);
      box->GetFlex(aState, flex);
      mBoxSize.Add(min, pref, max, ascent, flex, !isHorizontal); 

      node = node->GetNext();
    }
  }

  return mBoxSize;
}

// ------ nsMonumentLayout ------

nsMonumentLayout::nsMonumentLayout(nsIPresShell* aPresShell):nsSprocketLayout()
{
}

/*
PRBool 
nsMonumentLayout::GetInitialOrientation(PRBool& aIsHorizontal)
{
  aIsHorizontal = (mState & NS_STATE_IS_HORIZONTAL);
  return PR_TRUE;
}
*/

NS_IMETHODIMP
nsMonumentLayout::CastToTemple(nsTempleLayout** aTemple)
{
  *aTemple = nsnull;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMonumentLayout::CastToObelisk(nsObeliskLayout** aObelisk)
{
  *aObelisk = nsnull;
  return NS_ERROR_FAILURE;
}

/*
void
nsMonumentLayout::SetHorizontal(PRBool aIsHorizontal)
{
  if (aIsHorizontal) 
    mState |= NS_STATE_IS_HORIZONTAL;
  else
    mState &= ~NS_STATE_IS_HORIZONTAL;
}
*/

NS_IMETHODIMP
nsMonumentLayout::GetParentMonument(nsIBox* aBox, nsIMonument** aMonument)
{
  // go up and find our parent monument. Skip and non monument
  // parents.
  nsIBoxLayout* parent = this;
  *aMonument = nsnull;
  while (parent) {
    GetParentLayout(aBox, &parent);

    if (parent && NS_SUCCEEDED(parent->QueryInterface(NS_GET_IID(nsIMonument), (void**)aMonument)) && *aMonument) 
      return NS_OK;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMonumentLayout::GetOtherMonuments(nsIBox* aBox, nsBoxSizeList** aList)
{
  nsIMonument* parent;
  GetParentMonument(aBox, &parent);
  return parent->GetOtherMonumentsAt(aBox, 0, aList, this);
}

/**
 * Get the monuments in the other temple at the give monument index
 */
NS_IMETHODIMP
nsMonumentLayout::GetOtherMonumentsAt(nsIBox* aBox, PRInt32 aIndexOfObelisk, nsBoxSizeList** aList, nsMonumentLayout* aRequestor)
{
   PRInt32 index = 0;
   nsIBox* child = nsnull;
   aBox->GetChildBox(&child);
   PRInt32 count = 0;
   while(child)
   {
     nsIBoxLayout* layout = nsnull;
     child->GetLayoutManager(&layout);
     nsIMonument* monument = nsnull;
     if (layout == aRequestor) {
        index = count;
        break;
     }

     child->GetNextBox(&child);
     count++;
   }

   NS_ASSERTION(index != -1,"Error can't find requestor!!");
   aIndexOfObelisk += index;

   nsIMonument* parent;
   GetParentMonument(aBox, &parent);

   parent->GetOtherMonumentsAt(aBox, aIndexOfObelisk, aList, this);
   return NS_OK;
}

NS_IMETHODIMP
nsMonumentLayout::GetOtherTemple(nsIBox* aBox, nsTempleLayout** aTemple, nsIBox** aTempleBox, nsMonumentLayout* aRequestor)
{
   nsIMonument* parent;
   GetParentMonument(aBox, &parent);
   parent->GetOtherTemple(aBox, aTemple, aTempleBox, this);
   return NS_OK;
}

NS_IMETHODIMP
nsMonumentLayout::GetMonumentsAt(nsIBox* aBox, PRInt32 aMonumentIndex, nsBoxSizeList** aList)
{
  // create an info list for the given column.
  nsIBox* box = nsnull;
  aBox->GetChildBox(&box);
  PRInt32 count = 0;
  while(box) {
     if (count == aMonumentIndex)
     {
        *aList = new nsBoxSizeListNodeImpl(box);
        return NS_OK;
     }
     box->GetNextBox(&box);
     count++;
  }
  return NS_ERROR_FAILURE;
}

/*
NS_IMETHODIMP
nsMonumentLayout::CountMonuments(PRInt32& aCount)
{
  aCount = 1;
  return NS_OK;
}
*/

NS_IMETHODIMP
nsMonumentLayout::BuildBoxSizeList(nsIBox* aBox, nsBoxLayoutState& aState, nsBoxSize** aFirst, nsBoxSize** aLast)
{
   *aFirst = *aLast = new nsBoxSize();

   nsSize pref(0,0);
   nsSize min(0,0);
   nsSize max(NS_INTRINSICSIZE,NS_INTRINSICSIZE);
   nscoord flex = 0;
   nscoord ascent = 0;

   aBox->GetPrefSize(aState, pref);
   aBox->GetMinSize(aState, min);
   aBox->GetMaxSize(aState, max);
   aBox->GetAscent(aState, ascent);
   aBox->GetFlex(aState, flex);
   nsBox::BoundsCheck(min, pref, max);
   nsMargin borderPadding(0,0,0,0);
   aBox->GetBorderAndPadding(borderPadding);

   nsMargin margin(0,0,0,0);
   aBox->GetMargin(margin);

   PRBool isHorizontal = PR_FALSE;
   aBox->GetOrientation(isHorizontal);

   (*aFirst)->Add(min, pref, max, ascent, flex, isHorizontal); 
   (*aFirst)->Add(borderPadding,isHorizontal);
   (*aFirst)->Add(margin,isHorizontal);

   return NS_OK;
}

NS_IMETHODIMP
nsMonumentLayout::GetMonumentList(nsIBox* aBox, nsBoxLayoutState& aState, nsBoxSizeList** aList)
{
  nsIBox* child = nsnull;
  aBox->GetChildBox(&child);
  nsBoxSizeList* last = nsnull;
  while(child)
  {
    nsBoxSizeList* newOne = new nsBoxSizeListNodeImpl(child);
    if (*aList == nsnull) 
      *aList = last = newOne;
    else {
      last->SetNext(aState, newOne);
      last = newOne;
    }
  }

  return NS_OK;
}

NS_IMPL_ADDREF_INHERITED(nsMonumentLayout, nsBoxLayout);
NS_IMPL_RELEASE_INHERITED(nsMonumentLayout, nsBoxLayout);

NS_INTERFACE_MAP_BEGIN(nsMonumentLayout)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIMonument)
NS_INTERFACE_MAP_END_INHERITING(nsBoxLayout)
