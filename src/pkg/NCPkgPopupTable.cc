/*---------------------------------------------------------------------\
|                                                                      |
|                      __   __    ____ _____ ____                      |
|                      \ \ / /_ _/ ___|_   _|___ \                     |
|                       \ V / _` \___ \ | |   __) |                    |
|                        | | (_| |___) || |  / __/                     |
|                        |_|\__,_|____/ |_| |_____|                    |
|                                                                      |
|                               core system                            |
|                                                        (C) SuSE GmbH |
\----------------------------------------------------------------------/

   File:       NCPkgPopupTable.cc

   Author:     Gabriele Strattner <gs@suse.de>
   Maintainer: Michael Andres <ma@suse.de>

/-*/
#include "Y2Log.h"

#include "YMenuButton.h"
#include "YDialog.h"
#include "NCSplit.h"
#include "NCSpacing.h"
#include "NCPkgNames.h"
#include "NCPackageSelector.h"
#include "NCLabel.h"
#include "NCPushButton.h"
#include "NCPkgTable.h"

#include "NCZypp.h"
#include <zypp/ui/Selectable.h>
#include <zypp/ui/UserWantedPackages.h>

#include "NCPkgPopupTable.h"

using namespace std;

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPkgPopupTable::NCPkgPopupTable
//	METHOD TYPE : Constructor
//
//	DESCRIPTION :
//
NCPkgPopupTable::NCPkgPopupTable( const wpos at, NCPackageSelector * pkger )
    : NCPopup( at, false )
      , pkgTable( 0 )
      , okButton( 0 )
      , cancelButton( 0 )
      , packager( pkger )
{
    createLayout( );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPkgPopupTable::~NCPkgPopupTable
//	METHOD TYPE : Destructor
//
//	DESCRIPTION :
//
NCPkgPopupTable::~NCPkgPopupTable()
{
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPkgPopupTable::createLayout
//	METHOD TYPE : void
//
//	DESCRIPTION :
//
void NCPkgPopupTable::createLayout( )
{
    YWidgetOpt opt;

    // the vertical split is the (only) child of the dialog
    NCSplit * split = new NCSplit( this, opt, YD_VERT );
    addChild( split );

    split->addChild( new NCSpacing( split, opt, 0.6, false, true ) );

    // add the headline
    opt.isHeading.setValue( true );
    NCLabel * head = new NCLabel( split, opt, YCPString(NCPkgNames::AutoChangeLabel()) );
    split->addChild( head );

    split->addChild( new NCSpacing( split, opt, 0.6, false, true ) );

    opt.isHeading.setValue( false );
    NCLabel * lb1 = new NCLabel( split, opt, YCPString(NCPkgNames::AutoChangeText1()) );
    split->addChild( lb1 );
    NCLabel * lb2 = new NCLabel( split, opt, YCPString(NCPkgNames::AutoChangeText2()) );
    split->addChild( lb2 );
    
    // add the package table (use default type T_Packages)
    pkgTable = new NCPkgTable( split, opt );
    pkgTable->setPackager( packager );
    pkgTable->fillHeader();

    split->addChild( pkgTable );

    // HBox for the buttons
    NCSplit * hSplit = new NCSplit( split, opt, YD_HORIZ );
    split->addChild( hSplit );

    opt.isHStretchable.setValue( true );

    hSplit->addChild( new NCSpacing( hSplit, opt, 0.2, true, false ) );

    // add the OK button
    opt.key_Fxx.setValue( 10 );
    okButton = new NCPushButton( hSplit, opt, YCPString(NCPkgNames::OKLabel()) );
    okButton->setId( NCPkgNames::OkButton() );

    hSplit->addChild( okButton );
    hSplit->addChild( new NCSpacing( hSplit, opt, 0.4, true, false ) );

    // add the Cancel button
    opt.key_Fxx.setValue( 9 );
    cancelButton = new NCPushButton( hSplit, opt, YCPString(NCPkgNames::CancelLabel()) );
    cancelButton->setId( NCPkgNames::Cancel() );

    hSplit->addChild( cancelButton );
    hSplit->addChild( new NCSpacing( hSplit, opt, 0.2, true, false ) );

    split->addChild( new NCSpacing( split, opt, 0.6, false, true ) );
}


///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPkgPopupTable::fillAutoChanges
//	METHOD TYPE : bool
//
//	DESCRIPTION :
//
bool NCPkgPopupTable::fillAutoChanges( NCPkgTable * pkgTable )
{
    if ( !pkgTable )
	return false;

    pkgTable->itemsCleared();		// clear the table

    set<string> ignoredNames; 
    set<string> userWantedNames = zypp::ui::userWantedPackageNames();
    //these are the packages already selected for autoinstallation in previous 'verify system' run
    set<string> verifiedNames = packager->getVerifiedPkgs();

    //initialize storage for the new set
    insert_iterator< set<string> > result (ignoredNames, ignoredNames.begin());
    
    if(!verifiedNames.empty())
    { 
	//if we have some leftovers from previous run, do the union of the sets
	set_union(userWantedNames.begin(), userWantedNames.end(),
	           verifiedNames.begin(), verifiedNames.end(), result ); 	
    }
    else
	//else just take userWanted stuff
	ignoredNames = userWantedNames;

    for ( set<string>::iterator it = ignoredNames.begin(); it != ignoredNames.end(); ++it )
	NCMIL << "Ignoring: " << *it << endl;
	 
    ZyppPoolIterator
	b = zyppPkgBegin(),
	e = zyppPkgEnd(),
	it;

    for (it = b; it != e; ++it)
    {
	ZyppSel slb = *it;

	// show all packages which are automatically selected for installation
	if ( slb->toModify() && slb->modifiedBy () != zypp::ResStatus::USER )
	{
	    if ( ! contains( ignoredNames, slb->name() ) )
	    {
		ZyppPkg pkgPtr = tryCastToZyppPkg (slb->theObj());
		if ( pkgPtr )
		{
		    NCMIL << "The status of " << pkgPtr->name() << " has automatically changed" << endl;
		    pkgTable->createListEntry( pkgPtr, slb );
		    //also add to 'already verified' set
		    packager->insertVerifiedPkg( pkgPtr->name() );
		}
	    }
	}
    }

    pkgTable->drawList();

    if ( pkgTable->getNumLines() > 0 )
    {
	return true;
    }
    else
    {
	return false;
    }
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPkgPopupTable::showInfoPopup
//	METHOD TYPE : NCursesEvent event
//
//	DESCRIPTION :
//
NCursesEvent NCPkgPopupTable::showInfoPopup( )
{
    postevent = NCursesEvent();

    if ( !fillAutoChanges( pkgTable ) )
    {
	postevent = NCursesEvent::button;
	return postevent;
    }

    do {
	// show the popup
	popupDialog( );
    } while ( postAgain() );

    popdownDialog();

    return postevent;
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPkgPopupTable::niceSize
//	METHOD TYPE : void
//
//	DESCRIPTION :
//

long NCPkgPopupTable::nicesize(YUIDimension dim)
{
    return ( dim == YD_HORIZ ? NCurses::cols()-15 : NCurses::lines()-5 );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPopup::wHandleInput
//	METHOD TYPE : NCursesEvent
//
//	DESCRIPTION :
//
NCursesEvent NCPkgPopupTable::wHandleInput( wint_t ch )
{
    if ( ch == 27 ) // ESC
	return NCursesEvent::cancel;

    if ( ch == KEY_RETURN )
	return NCursesEvent::button;

    return NCDialog::wHandleInput( ch );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPkgPopupTable::postAgain
//	METHOD TYPE : bool
//
//	DESCRIPTION :
//
bool NCPkgPopupTable::postAgain()
{
    if ( ! postevent.widget )
	return false;

    YCPValue currentId =  dynamic_cast<YWidget *>(postevent.widget)->id();

    if ( !currentId.isNull()
	 && currentId->compare( NCPkgNames::Cancel() ) == YO_EQUAL )
    {
	//user hit cancel - discard set of changes (if not empty)
	packager->clearVerifiedPkgs();

	// close the dialog
	postevent = NCursesEvent::cancel;
    }

    if ( postevent == NCursesEvent::button || postevent == NCursesEvent::cancel )
    {
	// return false means: close the popup dialog
	return false;
    }
    return true;
}

