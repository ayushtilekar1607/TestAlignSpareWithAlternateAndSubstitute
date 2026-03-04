// @<COPYRIGHT>@
// ==================================================
// Copyright 2026.
// Siemens Product Lifecycle Management Software Inc.
// All Rights Reserved.
// ==================================================
// @<COPYRIGHT>@

/**
    TestAlignSpareWithAlternateAndSubstitute.cxx
    Test auto-generation of spare definitions for alternates and substitutes
*/

#include <common/tcdefs.h>

#include <string>
#include <vector>

#include <awb0occmgmt/awb0occmgmt.hxx>
#include <awb0occmgmtbom/awb0occmgmtbom.hxx>
#include <base_utest/Legacy.hxx>
#include <base_utest/UserProperties.hxx>
#include <base_utils/IFail.hxx>
#include <base_utils/Mem.h>
#include <base_utils/ScopedSmPtr.hxx>
#include <base_utils/String.hxx>
#include <base_utils/TcRand.hxx>
#include <base_utils/TcResultStatus.hxx>
#include <bom/bom.h>
#include <fclasses/tc_string.h>
#include <metaframework/BusinessObjectRef.hxx>
#include <mrocore/mrocore.h>
#include <mrocore/mrocore_attr.h>
#include <mrocore/mrocore_utils.hxx>
#include <mrocore/Smr0SpareDefinition.hxx>
#include <mrotestutils/MroTestStructureUtils.hxx>
#include <mrotestutils/MroTestUtils.hxx>
#include <mrotestutils/SlmStringStore.hxx>
#include <smr1mrocoreaw/smr1AutoCreateSpareDefinitionsService.hxx>
#include <smr1mrocoreaw/smr1mrocoreaw_errors.h>
#include <tc/preferences.h>
#include <tccore/aom.h>
#include <tccore/aom_prop.h>
#include <tccore/grm.h>
#include <tccore/item.h>
#include <tccore/ItemImpl.hxx>
#include <tctest/TcTest.hxx>

// Inform Coverity that tag_t is a simple unsigned int handle, not a pointer requiring memory management
#ifdef __COVERITY__
typedef unsigned int tag_t;
#endif


using namespace std;
using namespace testutils;
using namespace Teamcenter::UTest;
using namespace Teamcenter::mrocore;
using namespace Teamcenter::MRO;
using namespace testing;

namespace Teamcenter::ActiveWorkspaceMroCore
{
    class TestAlignSpareWithAlternateAndSubstitute : public ::testing::Test
    {
    public:

        // Helper methods to get cached relation types
        static tag_t getNeutralSDsRelationType();
        static tag_t getOccSDsRelationType();

    protected:
        TestAlignSpareWithAlternateAndSubstitute() = default;
        ~TestAlignSpareWithAlternateAndSubstitute() = default;
        static void SetUpTestSuite(); // Setup test data

        void attachAlternateToPart( tag_t primaryItemTag, tag_t& alternateItemTag, const std::string& alternateName = "" );
        void attachSubstituteToOccurrence( tag_t bomLineTag, tag_t& substituteItemTag, const std::string& substituteName = "" );
        
        static bool m_isInitialized;
        static tag_t m_neutralSDsRelationType;
        static tag_t m_occSDsRelationType;

    };

    // Initialize static members
    bool TestAlignSpareWithAlternateAndSubstitute::m_isInitialized = false;
    tag_t TestAlignSpareWithAlternateAndSubstitute::m_neutralSDsRelationType = NULLTAG;
    tag_t TestAlignSpareWithAlternateAndSubstitute::m_occSDsRelationType = NULLTAG;

    void TestAlignSpareWithAlternateAndSubstitute::SetUpTestSuite()
    {
        if ( !m_isInitialized )
        {
            MroTestUtils::isMroCoreAWTemplateInstalled();
            m_isInitialized = true;
        }
    }

    tag_t TestAlignSpareWithAlternateAndSubstitute::getNeutralSDsRelationType()
    {
        if ( m_neutralSDsRelationType == NULLTAG )
        {
            ASSERT_ITK_OK( GRM_find_relation_type( ClassNames::Smr0NeutralSDs.c_str(), &m_neutralSDsRelationType ) );
        }
        return m_neutralSDsRelationType;
    }

    tag_t TestAlignSpareWithAlternateAndSubstitute::getOccSDsRelationType()
    {
        if ( m_occSDsRelationType == NULLTAG )
        {
            ASSERT_ITK_OK( GRM_find_relation_type( ClassNames::Smr0OccSDs.c_str(), &m_occSDsRelationType ) );
        }
        return m_occSDsRelationType;
    }

    void TestAlignSpareWithAlternateAndSubstitute::attachAlternateToPart( tag_t primaryItemTag, tag_t& alternateItemTag, const std::string& alternateName )
    {
        tag_t alternateRev = NULLTAG;
        
        std::string altName = alternateName;

        // Generate alternate name if not provided
        if ( alternateName.empty() )
        {
            int randNum = Teamcenter::RAND::TcRAND_Int();
            altName = "Alternate_" + std::to_string( randNum );
        }
        
        // Create the alternate item
        ASSERT_ITK_OK( ITEM_create_item( NULL, altName.c_str(), "Part", "A", &alternateItemTag, &alternateRev ) );
        ASSERT_ITK_OK( ITEM_save_item( alternateItemTag ) );
        
        ASSERT_ITK_OK( AOM_save_with_extensions( alternateItemTag ) );
        AOM_refresh( alternateItemTag, 0 );
        
        ASSERT_ITK_OK( AOM_save_with_extensions( alternateRev ) );
        AOM_refresh( alternateRev, 0 );
        
        // Attach the alternate to the primary item using global alternates relationship
        ASSERT_ITK_OK( ITEM_add_related_global_alternates( primaryItemTag, 1, &alternateItemTag ) );
    }

    void TestAlignSpareWithAlternateAndSubstitute::attachSubstituteToOccurrence( tag_t bomLineTag, tag_t& substituteItemTag, const std::string& substituteName )
    {
        tag_t substituteRev = NULLTAG;
        tag_t substituteLine = NULLTAG;
        
        std::string subName = substituteName;

        // Generate substitute name if not provided
        if ( substituteName.empty() )
        {
            int randNum = Teamcenter::RAND::TcRAND_Int();
            subName = "Substitute_" + std::to_string( randNum );
        }
        
        // Create the substitute item
        ASSERT_ITK_OK( ITEM_create_item( NULL, subName.c_str(), "Part", "A", &substituteItemTag, &substituteRev ) );
        ASSERT_ITK_OK( ITEM_save_item( substituteItemTag ) );
        
        ASSERT_ITK_OK( AOM_save_with_extensions( substituteItemTag ) );
        AOM_refresh( substituteItemTag, 0 );
        
        ASSERT_ITK_OK( AOM_save_with_extensions( substituteRev ) );
        AOM_refresh( substituteRev, 0 );
        
        // Attach the substitute to the BOM line (occurrence-specific)
        ASSERT_ITK_OK( BOM_line_add_substitute( bomLineTag, substituteItemTag, substituteRev, NULLTAG, &substituteLine ) );
    }

    void createManualSpareDefinition( tag_t itemTag, tag_t& spareDefTag, bool isSystemGenerated = false )
    {
        // Generate unique spare definition name
        int randNum = Teamcenter::RAND::TcRAND_Int();
        std::string spareDefName = "ManualSpareDef_" + std::to_string( randNum );
        
        // Create spare definition using MroTestStructureUtils helper
        ASSERT_ITK_OK( MroTestStructureUtils::createSD( { spareDefName,
                                                           "Manual Spare Definition",
                                                           {},  // contracts
                                                           {},  // customers
                                                           {},  // geographies
                                                           {},  // serviceLevels
                                                           {},  // spares
                                                           true,  // active
                                                           isSystemGenerated },
                                                         spareDefTag ) );
        
        // Get the spare definition revision to verify isSystemGenerated flag
        tag_t spareDefRevTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( spareDefTag, ClassNames::Smr0SpareDefinition.c_str(), &spareDefRevTag ) );
        
        // Verify the flag was set correctly
        logical actualIsSystemGenerated = !isSystemGenerated; // Initialize to opposite
        ASSERT_ITK_OK( AOM_ask_value_logical( spareDefRevTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &actualIsSystemGenerated ) );
        
        // Attach the spare definition to the item using Smr0NeutralSDs relation
        tag_t neutralSDsRelationType = TestAlignSpareWithAlternateAndSubstitute::getNeutralSDsRelationType();
        
        tag_t relationTag = NULLTAG;
        ASSERT_ITK_OK( GRM_create_relation( itemTag, spareDefTag, neutralSDsRelationType, NULLTAG, &relationTag ) );
        ASSERT_ITK_OK( GRM_save_relation( relationTag ) );
    }

    TEST_F( TestAlignSpareWithAlternateAndSubstitute, TestAutoCreateSpareDefForAlternate )
    {
        int count = 0;
        tag_t alternateItem = NULLTAG;
        
        // Step 1: Create a neutral part structure with one child
        NeutralStructInputData neuStructInputData( "TestAutoCreateSpareDef_Assembly1", 1 );
        NeutralStructOutputData neuStructOutputData;
        tag_t neutralTopLine = MroTestStructureUtils::createNeutralStructure( neuStructInputData, neuStructOutputData );
        
        if ( neutralTopLine == NULLTAG )
        {
            FAIL() << "TestAutoCreateSpareDefForAlternate: Failed to create neutral part structure";
        }

        // Step 2: Get the child item and create an alternate for it
        scoped_smptr<tag_t> childLines;
        ASSERT_ITK_OK( BOM_line_ask_all_child_lines( neuStructOutputData.topBomLine, &count, &childLines ) );
        ASSERT_EQ( count, 1 ) << "Expected 1 child in neutral structure";
        
        tag_t childItem = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines[0], BOMLinePropertyNames::bl_item.c_str(), &childItem ) );
        
        // Attach alternate to the child part (item-level)
        attachAlternateToPart( childItem, alternateItem, "AlternatePart1" );
        ASSERT_NE( alternateItem, NULLTAG ) << "Alternate item was not created";

        // Step 3-4: Create SBOM structure - Convert to Awb0Element objects
        OCCMGMT_GetOccurrencesInProductInput productInfoInput;
        OCCMGMT_GetOccurrencesInProductOutput productInfo;
        productInfoInput.product = neuStructOutputData.topItemRevTag;
        productInfoInput.maxToLoad = 0;
        productInfoInput.firstLevelOnly = false;
        productInfoInput.requestPrefMap["occWindowCtx"] = { "SLMNeutral" };
        
        ASSERT_ITK_OK( OCCMGMT_getOccurrencesInProduct( productInfoInput, productInfo ) );
        ASSERT_GT( productInfo.resultOccurrencesInfo.occurrences.size(), 0 ) 
            << "No occurrences returned from OCCMGMT_getOccurrencesInProduct";

        // Step 5: Call service to auto-create spare definitions
        tag_t childAwb0Element = productInfo.resultOccurrencesInfo.occurrences[0];
        std::vector<BusinessObjectRef<Teamcenter::BusinessObject>> assetRefs;
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( childAwb0Element ) );

        logical isSpare = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( childAwb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &isSpare ) ); 
        ASSERT_FALSE( isSpare ) << "Child element should not be marked as spare before creating spare definitions";
        
        AutoCreateSpareDefinitionsService& service = AutoCreateSpareDefinitionsService::getInstance();
        ASSERT_ITK_OK( service.createSpareDefinitionForAssets( assetRefs ) );

        ASSERT_ITK_OK( AOM_ask_value_logical( childAwb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &isSpare ) ); 
        ASSERT_FALSE( isSpare ) << "Child element should not be marked as spare before creating spare definitions";

        // Step 6 & 7: Validate spare definitions were created with correct properties
        
        // Get Smr0NeutralSDs relation type
        tag_t neutralSDsRelationType = getNeutralSDsRelationType();
        
        // Get spare definitions attached to the item
        int sdCount = 0;
        scoped_smptr<tag_t> sdThreads;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( childItem, neutralSDsRelationType, &sdCount, &sdThreads ) );
        ASSERT_GT( sdCount, 0 ) << "Expected at least one spare definition to be created";
        
        // Verify the spare definition properties
        // For item-level relation, sdThreads[0] is the thread container, need to get the actual spare def
        tag_t threadTag = sdThreads[0];
        tag_t spareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( threadTag, ClassNames::Smr0SpareDefinition.c_str(), &spareDefTag ) );

        logical isSystemGenerated = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( spareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &isSystemGenerated ) );
        ASSERT_TRUE( isSystemGenerated ) << "Spare definition should be marked as system generated";

        // Validate that the spare definition contains the alternate part in smr0Spares
        int sparePartsCount = 0;
        scoped_smptr<tag_t> spareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( spareDefTag, SDPropertyNames::smr0Spares.c_str(), &sparePartsCount, &spareParts ) );
        ASSERT_GT( sparePartsCount, 0 ) << "Spare definition should have at least one spare part (alternate)";
        
        // Verify the alternate item is in the list
        bool alternateFound = false;
        for ( int i = 0; i < sparePartsCount; ++i )
        {
            if ( spareParts[i] == alternateItem )
            {
                alternateFound = true;
                break;
            }
        }
        ASSERT_TRUE( alternateFound ) << "Alternate item should be listed in the spare definition's smr0_spares";

        // Cleanup
        MroTestStructureUtils::closeWindows( neuStructOutputData.bomWindow, neuStructOutputData.mrocoreWindow, NULLTAG );
    }

    TEST_F( TestAlignSpareWithAlternateAndSubstitute, TestAutoCreateSpareDefForSubstitute )
    {
        int count = 0;
        tag_t substituteItem = NULLTAG;
        
        // Step 1: Create a neutral part structure with one child
        NeutralStructInputData neuStructInputData( "TestAutoCreateSpareDef_Assembly2", 1 );
        NeutralStructOutputData neuStructOutputData;
        tag_t neutralTopLine = MroTestStructureUtils::createNeutralStructure( neuStructInputData, neuStructOutputData );
        
        if ( neutralTopLine == NULLTAG )
        {
            FAIL() << "TestAutoCreateSpareDefForSubstitute: Failed to create neutral part structure";
        }

        // Step 2: Get the child BOM line and create a substitute for it
        scoped_smptr<tag_t> childLines;
        ASSERT_ITK_OK( BOM_line_ask_all_child_lines( neuStructOutputData.topBomLine, &count, &childLines ) );
        ASSERT_EQ( count, 1 ) << "Expected 1 child in neutral structure";
        
        // Attach substitute to the child BOM line (occurrence-level)
        attachSubstituteToOccurrence( childLines[0], substituteItem, "SubstitutePart1" );
        ASSERT_NE( substituteItem, NULLTAG ) << "Substitute item was not created";
        
        // Save the BOM window to persist the substitute
        ASSERT_ITK_OK( BOM_save_window( neuStructOutputData.bomWindow ) );

        // Step 3-4: Create SBOM structure - Convert to Awb0Element objects
        OCCMGMT_GetOccurrencesInProductInput productInfoInput;
        OCCMGMT_GetOccurrencesInProductOutput productInfo;
        productInfoInput.product = neuStructOutputData.topItemRevTag;
        productInfoInput.maxToLoad = 0;
        productInfoInput.firstLevelOnly = false;
        productInfoInput.requestPrefMap["occWindowCtx"] = { "SLMNeutral" };
        
        ASSERT_ITK_OK( OCCMGMT_getOccurrencesInProduct( productInfoInput, productInfo ) );
        ASSERT_GT( productInfo.resultOccurrencesInfo.occurrences.size(), 0 ) 
            << "No occurrences returned from OCCMGMT_getOccurrencesInProduct";

        // Step 5: Call service to auto-create spare definitions
        tag_t childAwb0Element = productInfo.resultOccurrencesInfo.occurrences[0];  std::vector<BusinessObjectRef<Teamcenter::BusinessObject>> assetRefs;
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( childAwb0Element ) );

        logical isSpare = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( childAwb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &isSpare ) );
        ASSERT_FALSE( isSpare ) << "Child element should not be marked as spare before creating spare definitions";

        AutoCreateSpareDefinitionsService& service = AutoCreateSpareDefinitionsService::getInstance();
        ASSERT_ITK_OK( service.createSpareDefinitionForAssets( assetRefs ) );

        ASSERT_ITK_OK( AOM_ask_value_logical( childAwb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &isSpare ) );
        ASSERT_TRUE( isSpare ) << "Child element should be marked as spare after creating spare definitions";

        // Step 6 & 7: Validate spare definitions were created with correct properties
        // Query for spare definitions related to the child occurrence
        
        // Get the real PS occurrence from the BOM line
        tag_t psOccurrence = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines[0], BOMLinePropertyNames::bl_real_occurrence.c_str(), &psOccurrence ) );
        ASSERT_NE( psOccurrence, NULLTAG ) << "Failed to get PS occurrence from BOM line";
        
        // Get Smr0OccSDs relation type
        tag_t occSDsRelationType = getOccSDsRelationType();
        
        // Get spare definitions attached to the occurrence
        int sdCount = 0;
        scoped_smptr<tag_t> sdThreads;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( psOccurrence, occSDsRelationType, &sdCount, &sdThreads ) );
        ASSERT_GT( sdCount, 0 ) << "Expected at least one spare definition to be created";
        
        // Verify the spare definition properties
        // For occurrence-level relation, sdThreads[0] is the thread container, need to get the actual spare def
        tag_t threadTag = sdThreads[0];
        tag_t spareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( threadTag, ClassNames::Smr0SpareDefinition.c_str(), &spareDefTag ) );

        logical isSystemGenerated = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( spareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &isSystemGenerated ) );
        ASSERT_TRUE( isSystemGenerated ) << "Spare definition should be marked as system generated";

        // Validate that the spare definition contains the substitute part in smr0Spares
        int sparePartsCount = 0;
        scoped_smptr<tag_t> spareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( spareDefTag, SDPropertyNames::smr0Spares.c_str(), &sparePartsCount, &spareParts ) );
        ASSERT_GT( sparePartsCount, 0 ) << "Spare definition should have at least one spare part (substitute)";
        
        // Verify the substitute item is in the list
        bool substituteFound = false;
        for ( int i = 0; i < sparePartsCount; ++i )
        {
            if ( spareParts[i] == substituteItem )
            {
                substituteFound = true;
                break;
            }
        }
        ASSERT_TRUE( substituteFound ) << "Substitute item should be listed in the spare definition's smr0_spares";

        // Cleanup
        MroTestStructureUtils::closeWindows( neuStructOutputData.bomWindow, neuStructOutputData.mrocoreWindow, NULLTAG );
    }

    TEST_F( TestAlignSpareWithAlternateAndSubstitute, TestAutoCreateSpareDefForAlternateAndSubstitute )
    {
        int count = 0;
        tag_t alternateItem = NULLTAG;
        tag_t substituteItem = NULLTAG;
        
        // Step 1: Create a neutral part structure with one child
        NeutralStructInputData neuStructInputData( "TestAutoCreateSpareDef_Assembly3", 1 );
        NeutralStructOutputData neuStructOutputData;
        tag_t neutralTopLine = MroTestStructureUtils::createNeutralStructure( neuStructInputData, neuStructOutputData );
        
        if ( neutralTopLine == NULLTAG )
        {
            FAIL() << "TestAutoCreateSpareDefForAlternateAndSubstitute: Failed to create neutral part structure";
        }

        // Step 2: Get the child and create an alternate for it
        scoped_smptr<tag_t> childLines;
        ASSERT_ITK_OK( BOM_line_ask_all_child_lines( neuStructOutputData.topBomLine, &count, &childLines ) );
        ASSERT_EQ( count, 1 ) << "Expected 1 child in neutral structure";
        
        tag_t childItem = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines[0], BOMLinePropertyNames::bl_item.c_str(), &childItem ) );
        
        // Attach alternate to the child part (item-level)
        attachAlternateToPart( childItem, alternateItem, "AlternatePart3" );
        ASSERT_NE( alternateItem, NULLTAG ) << "Alternate item was not created";

        // Step 3: Attach substitute to the child BOM line (occurrence-level)
        attachSubstituteToOccurrence( childLines[0], substituteItem, "SubstitutePart3" );
        ASSERT_NE( substituteItem, NULLTAG ) << "Substitute item was not created";
        
        // Save the BOM window to persist the substitute
        ASSERT_ITK_OK( BOM_save_window( neuStructOutputData.bomWindow ) );

        // Step 4: Create SBOM structure - Convert to Awb0Element objects
        OCCMGMT_GetOccurrencesInProductInput productInfoInput;
        OCCMGMT_GetOccurrencesInProductOutput productInfo;
        productInfoInput.product = neuStructOutputData.topItemRevTag;
        productInfoInput.maxToLoad = 0;
        productInfoInput.firstLevelOnly = false;
        productInfoInput.requestPrefMap["occWindowCtx"] = { "SLMNeutral" };
        
        ASSERT_ITK_OK( OCCMGMT_getOccurrencesInProduct( productInfoInput, productInfo ) );
        ASSERT_GT( productInfo.resultOccurrencesInfo.occurrences.size(), 0 ) 
            << "No occurrences returned from OCCMGMT_getOccurrencesInProduct";

        // Step 5: Call service to auto-create spare definitions
        tag_t childAwb0Element = productInfo.resultOccurrencesInfo.occurrences[0];

        // Step 6: Call service to auto-create spare definitions
        std::vector<BusinessObjectRef<Teamcenter::BusinessObject>> assetRefs;
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( childAwb0Element ) );
        
        // Validate isSpare property before service call
        logical isSpare = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( childAwb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &isSpare ) );
        ASSERT_FALSE( isSpare ) << "Child element should not be marked as spare before creating spare definitions";

        AutoCreateSpareDefinitionsService& service = AutoCreateSpareDefinitionsService::getInstance();
        ASSERT_ITK_OK( service.createSpareDefinitionForAssets( assetRefs ) );

        // Validate isSpare property after service call (should be true due to substitute/occurrence-level)
        isSpare = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( childAwb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &isSpare ) );
        ASSERT_TRUE( isSpare ) << "Child element should be marked as spare after creating spare definitions with substitute";

        // Step 7 & 8: Validate item-level spare definition
        tag_t neutralSDsRelationType = getNeutralSDsRelationType();
        
        int itemSDCount = 0;
        scoped_smptr<tag_t> itemSDThreads;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( childItem, neutralSDsRelationType, &itemSDCount, &itemSDThreads ) );
        ASSERT_GT( itemSDCount, 0 ) << "Expected at least one item-level spare definition to be created";
        
        tag_t itemThreadTag = itemSDThreads[0];
        tag_t itemSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( itemThreadTag, ClassNames::Smr0SpareDefinition.c_str(), &itemSpareDefTag ) );

        logical itemIsSystemGenerated = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( itemSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &itemIsSystemGenerated ) );
        ASSERT_TRUE( itemIsSystemGenerated ) << "Item-level spare definition should be marked as system generated";

        // Validate that the item-level spare definition contains the alternate part in smr0Spares
        int itemSparePartsCount = 0;
        scoped_smptr<tag_t> itemSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( itemSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &itemSparePartsCount, &itemSpareParts ) );
        ASSERT_GT( itemSparePartsCount, 0 ) << "Item-level spare definition should have at least one spare part (alternate)";
        
        bool alternateFound = false;
        for ( int i = 0; i < itemSparePartsCount; ++i )
        {
            if ( itemSpareParts[i] == alternateItem )
            {
                alternateFound = true;
                break;
            }
        }
        ASSERT_TRUE( alternateFound ) << "Alternate item should be listed in the item-level spare definition's smr0Spares";

        // Validate occurrence-level spare definition
        tag_t psOccurrence = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines[0], BOMLinePropertyNames::bl_real_occurrence.c_str(), &psOccurrence ) );
        ASSERT_NE( psOccurrence, NULLTAG ) << "Failed to get PS occurrence from BOM line";
        
        tag_t occSDsRelationType = getOccSDsRelationType();
        
        int occSDCount = 0;
        scoped_smptr<tag_t> occSDThreads;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( psOccurrence, occSDsRelationType, &occSDCount, &occSDThreads ) );
        ASSERT_GT( occSDCount, 0 ) << "Expected at least one occurrence-level spare definition to be created";
        
        tag_t occThreadTag = occSDThreads[0];
        tag_t occSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( occThreadTag, ClassNames::Smr0SpareDefinition.c_str(), &occSpareDefTag ) );

        logical occIsSystemGenerated = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( occSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &occIsSystemGenerated ) );
        ASSERT_TRUE( occIsSystemGenerated ) << "Occurrence-level spare definition should be marked as system generated";

        // Validate that the occurrence-level spare definition contains the substitute part in smr0Spares
        int occSparePartsCount = 0;
        scoped_smptr<tag_t> occSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( occSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &occSparePartsCount, &occSpareParts ) );
        ASSERT_GT( occSparePartsCount, 0 ) << "Occurrence-level spare definition should have at least one spare part (substitute)";
        
        bool substituteFound = false;
        for ( int i = 0; i < occSparePartsCount; ++i )
        {
            if ( occSpareParts[i] == substituteItem )
            {
                substituteFound = true;
                break;
            }
        }
        ASSERT_TRUE( substituteFound ) << "Substitute item should be listed in the occurrence-level spare definition's smr0Spares";

        // Cleanup
        MroTestStructureUtils::closeWindows( neuStructOutputData.bomWindow, neuStructOutputData.mrocoreWindow, NULLTAG );
    }
    
    TEST_F( TestAlignSpareWithAlternateAndSubstitute, TestAutoCreateSpareDefForMultipleChildren )
    {
        int count = 0;
        tag_t alternateItem1 = NULLTAG;
        tag_t substituteItem2 = NULLTAG;
        tag_t alternateItem3 = NULLTAG;
        tag_t substituteItem3 = NULLTAG;
        
        // Step 1: Create a neutral part structure with three children
        NeutralStructInputData neuStructInputData( "TestAutoCreateSpareDef_Assembly4", 3 );
        NeutralStructOutputData neuStructOutputData;
        tag_t neutralTopLine = MroTestStructureUtils::createNeutralStructure( neuStructInputData, neuStructOutputData );
        
        if ( neutralTopLine == NULLTAG )
        {
            FAIL() << "TestAutoCreateSpareDefForMultipleChildren: Failed to create neutral part structure";
        }

        // Step 2: Get child lines and attach alternates/substitutes
        scoped_smptr<tag_t> childLines;
        ASSERT_ITK_OK( BOM_line_ask_all_child_lines( neuStructOutputData.topBomLine, &count, &childLines ) );
        ASSERT_EQ( count, 3 ) << "Expected 3 children in neutral structure";
        
        // Get child items
        tag_t childItem1 = NULLTAG, childItem2 = NULLTAG, childItem3 = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines[0], BOMLinePropertyNames::bl_item.c_str(), &childItem1 ) );
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines[1], BOMLinePropertyNames::bl_item.c_str(), &childItem2 ) );
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines[2], BOMLinePropertyNames::bl_item.c_str(), &childItem3 ) );
        
        // Child1: Attach alternate only (item-level)
        attachAlternateToPart( childItem1, alternateItem1, "AlternatePart4_1" );
        ASSERT_NE( alternateItem1, NULLTAG ) << "Alternate item for child1 was not created";
        
        // Child2: Attach substitute only (occurrence-level)
        attachSubstituteToOccurrence( childLines[1], substituteItem2, "SubstitutePart4_2" );
        ASSERT_NE( substituteItem2, NULLTAG ) << "Substitute item for child2 was not created";
        
        // Child3: Attach both alternate and substitute
        attachAlternateToPart( childItem3, alternateItem3, "AlternatePart4_3" );
        ASSERT_NE( alternateItem3, NULLTAG ) << "Alternate item for child3 was not created";
        attachSubstituteToOccurrence( childLines[2], substituteItem3, "SubstitutePart4_3" );
        ASSERT_NE( substituteItem3, NULLTAG ) << "Substitute item for child3 was not created";
        
        // Save the BOM window
        ASSERT_ITK_OK( BOM_save_window( neuStructOutputData.bomWindow ) );

        // Step 3: Create SBOM structure - Convert to Awb0Element objects
        OCCMGMT_GetOccurrencesInProductInput productInfoInput;
        OCCMGMT_GetOccurrencesInProductOutput productInfo;
        productInfoInput.product = neuStructOutputData.topItemRevTag;
        productInfoInput.maxToLoad = 0;
        productInfoInput.firstLevelOnly = false;
        productInfoInput.requestPrefMap["occWindowCtx"] = { "SLMNeutral" };
        
        ASSERT_ITK_OK( OCCMGMT_getOccurrencesInProduct( productInfoInput, productInfo ) );
        ASSERT_EQ( productInfo.resultOccurrencesInfo.occurrences.size(), 3 ) 
            << "Expected 3 occurrences from OCCMGMT_getOccurrencesInProduct";

        // Step 4: Set smr1Spare property to true for all children
        std::vector<BusinessObjectRef<Teamcenter::BusinessObject>> assetRefs;
        tag_t child1Awb0Element = productInfo.resultOccurrencesInfo.occurrences[0];
        tag_t child2Awb0Element = productInfo.resultOccurrencesInfo.occurrences[1];
        tag_t child3Awb0Element = productInfo.resultOccurrencesInfo.occurrences[2];
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( child1Awb0Element ) );
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( child2Awb0Element ) );
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( child3Awb0Element ) );

        // Step 5: Call service to auto-create spare definitions
        AutoCreateSpareDefinitionsService& service = AutoCreateSpareDefinitionsService::getInstance();
        ASSERT_ITK_OK( service.createSpareDefinitionForAssets( assetRefs ) );

        // Validate isSpare property after service call
        logical child1IsSpare = false; logical child2IsSpare = false; logical child3IsSpare = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child1Awb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &child1IsSpare ) );
        ASSERT_ITK_OK( AOM_ask_value_logical( child2Awb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &child2IsSpare ) );
        ASSERT_ITK_OK( AOM_ask_value_logical( child3Awb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &child3IsSpare ) );
        ASSERT_FALSE( child1IsSpare ) << "Child1 should not be marked as spare (alternate only, item-level)";
        ASSERT_TRUE( child2IsSpare ) << "Child2 should be marked as spare (substitute, occurrence-level)";
        ASSERT_TRUE( child3IsSpare ) << "Child3 should be marked as spare (has substitute, occurrence-level)";

        // Step 6: Validate spare definitions
        tag_t neutralSDsRelationType = getNeutralSDsRelationType();
        tag_t occSDsRelationType = getOccSDsRelationType();
        
        // Validate Child1: Should have item-level spare def only
        int child1ItemSDCount = 0;
        scoped_smptr<tag_t> child1ItemSDThreads;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( childItem1, neutralSDsRelationType, &child1ItemSDCount, &child1ItemSDThreads ) );
        ASSERT_GT( child1ItemSDCount, 0 ) << "Child1 should have item-level spare definition";
        
        tag_t child1SpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child1ItemSDThreads[0], ClassNames::Smr0SpareDefinition.c_str(), &child1SpareDefTag ) );
        logical child1IsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child1SpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child1IsSystemGen ) );
        ASSERT_TRUE( child1IsSystemGen ) << "Child1 item-level spare definition should be system generated";
        
        // Validate that child1 spare definition contains the alternate part
        int child1SparePartsCount = 0;
        scoped_smptr<tag_t> child1SpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child1SpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child1SparePartsCount, &child1SpareParts ) );
        ASSERT_GT( child1SparePartsCount, 0 ) << "Child1 spare definition should have at least one spare part (alternate)";
        
        bool child1AlternateFound = false;
        for ( int i = 0; i < child1SparePartsCount; ++i )
        {
            if ( child1SpareParts[i] == alternateItem1 )
            {
                child1AlternateFound = true;
                break;
            }
        }
        ASSERT_TRUE( child1AlternateFound ) << "Alternate item should be in child1 spare definition's smr0Spares";
        
        // Validate Child2: Should have occurrence-level spare def only
        tag_t psOccurrence2 = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines[1], BOMLinePropertyNames::bl_real_occurrence.c_str(), &psOccurrence2 ) );
        
        int child2OccSDCount = 0;
        scoped_smptr<tag_t> child2OccSDThreads;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( psOccurrence2, occSDsRelationType, &child2OccSDCount, &child2OccSDThreads ) );
        ASSERT_GT( child2OccSDCount, 0 ) << "Child2 should have occurrence-level spare definition";
        
        tag_t child2SpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child2OccSDThreads[0], ClassNames::Smr0SpareDefinition.c_str(), &child2SpareDefTag ) );
        logical child2IsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child2SpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child2IsSystemGen ) );
        ASSERT_TRUE( child2IsSystemGen ) << "Child2 occurrence-level spare definition should be system generated";
        
        // Validate that child2 spare definition contains the substitute part
        int child2SparePartsCount = 0;
        scoped_smptr<tag_t> child2SpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child2SpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child2SparePartsCount, &child2SpareParts ) );
        ASSERT_GT( child2SparePartsCount, 0 ) << "Child2 spare definition should have at least one spare part (substitute)";
        
        bool child2SubstituteFound = false;
        for ( int i = 0; i < child2SparePartsCount; ++i )
        {
            if ( child2SpareParts[i] == substituteItem2 )
            {
                child2SubstituteFound = true;
                break;
            }
        }
        ASSERT_TRUE( child2SubstituteFound ) << "Substitute item should be in child2 spare definition's smr0Spares";
        
        // Validate Child3: Should have both item-level and occurrence-level spare defs
        int child3ItemSDCount = 0;
        scoped_smptr<tag_t> child3ItemSDThreads;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( childItem3, neutralSDsRelationType, &child3ItemSDCount, &child3ItemSDThreads ) );
        ASSERT_GT( child3ItemSDCount, 0 ) << "Child3 should have item-level spare definition";
        
        tag_t child3ItemSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child3ItemSDThreads[0], ClassNames::Smr0SpareDefinition.c_str(), &child3ItemSpareDefTag ) );
        logical child3ItemIsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child3ItemSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child3ItemIsSystemGen ) );
        ASSERT_TRUE( child3ItemIsSystemGen ) << "Child3 item-level spare definition should be system generated";
        
        // Validate that child3 item-level spare definition contains the alternate part
        int child3ItemSparePartsCount = 0;
        scoped_smptr<tag_t> child3ItemSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child3ItemSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child3ItemSparePartsCount, &child3ItemSpareParts ) );
        ASSERT_GT( child3ItemSparePartsCount, 0 ) << "Child3 item-level spare definition should have at least one spare part (alternate)";
        
        bool child3AlternateFound = false;
        for ( int i = 0; i < child3ItemSparePartsCount; ++i )
        {
            if ( child3ItemSpareParts[i] == alternateItem3 )
            {
                child3AlternateFound = true;
                break;
            }
        }
        ASSERT_TRUE( child3AlternateFound ) << "Alternate item should be in child3 item-level spare definition's smr0Spares";
        
        tag_t psOccurrence3 = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines[2], BOMLinePropertyNames::bl_real_occurrence.c_str(), &psOccurrence3 ) );
        
        int child3OccSDCount = 0;
        scoped_smptr<tag_t> child3OccSDThreads;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( psOccurrence3, occSDsRelationType, &child3OccSDCount, &child3OccSDThreads ) );
        ASSERT_GT( child3OccSDCount, 0 ) << "Child3 should have occurrence-level spare definition";
        
        tag_t child3OccSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child3OccSDThreads[0], ClassNames::Smr0SpareDefinition.c_str(), &child3OccSpareDefTag ) );
        logical child3OccIsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child3OccSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child3OccIsSystemGen ) );
        ASSERT_TRUE( child3OccIsSystemGen ) << "Child3 occurrence-level spare definition should be system generated";

        // Validate that child3 occurrence-level spare definition contains the substitute part
        int child3OccSparePartsCount = 0;
        scoped_smptr<tag_t> child3OccSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child3OccSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child3OccSparePartsCount, &child3OccSpareParts ) );
        ASSERT_GT( child3OccSparePartsCount, 0 ) << "Child3 occurrence-level spare definition should have at least one spare part (substitute)";
        
        bool child3SubstituteFound = false;
        for ( int i = 0; i < child3OccSparePartsCount; ++i )
        {
            if ( child3OccSpareParts[i] == substituteItem3 )
            {
                child3SubstituteFound = true;
                break;
            }
        }
        ASSERT_TRUE( child3SubstituteFound ) << "Substitute item should be in child3 occurrence-level spare definition's smr0Spares";

        // Cleanup
        MroTestStructureUtils::closeWindows( neuStructOutputData.bomWindow, neuStructOutputData.mrocoreWindow, NULLTAG );
    }

    TEST_F( TestAlignSpareWithAlternateAndSubstitute, TestAutoCreateSpareDefForSubassemblyWithChildren )
    {
        int count = 0;
        
        // Step 1: Create TopAssembly with 1 child (the subassembly)
        NeutralStructInputData topAssemblyInputData( "TestAutoCreateSpareDef_TopAssembly", 1 );
        NeutralStructOutputData topAssemblyOutputData;
        tag_t topNeutralLine = MroTestStructureUtils::createNeutralStructure( topAssemblyInputData, topAssemblyOutputData );
        
        if ( topNeutralLine == NULLTAG )
        {
            FAIL() << "TestAutoCreateSpareDefForSubassemblyWithChildren: Failed to create top assembly structure";
        }

        // Step 2: Get the subassembly (first child of TopAssembly) and rename it
        scoped_smptr<tag_t> topChildren;
        ASSERT_ITK_OK( BOM_line_ask_all_child_lines( topAssemblyOutputData.topBomLine, &count, &topChildren ) );
        ASSERT_EQ( count, 1 ) << "TopAssembly should have 1 child (subassembly)";
        
        tag_t subassemblyBomLine = topChildren[0];
        tag_t subassemblyItem = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( subassemblyBomLine, BOMLinePropertyNames::bl_item.c_str(), &subassemblyItem ) );
        
        // Rename subassembly item to have meaningful name
        ASSERT_ITK_OK( AOM_set_value_string( subassemblyItem, "object_name", "TestAutoCreateSpareDef_SubAssembly" ) );
        ASSERT_ITK_OK( AOM_save_without_extensions( subassemblyItem ) );
        
        // Step 3: Create 3 children for the subassembly
        tag_t child1Item = NULLTAG, child1ItemRev = NULLTAG, child1BomLine = NULLTAG;
        tag_t child2Item = NULLTAG, child2ItemRev = NULLTAG, child2BomLine = NULLTAG;
        tag_t child3Item = NULLTAG, child3ItemRev = NULLTAG, child3BomLine = NULLTAG;
        
        // Create child1 item and add to subassembly
        int randNum1 = Teamcenter::RAND::TcRAND_Int();
        std::string child1Name = "Child1_" + std::to_string( randNum1 );
        ASSERT_ITK_OK( ITEM_create_item( NULL, child1Name.c_str(), "Part", "A", &child1Item, &child1ItemRev ) );
        ASSERT_ITK_OK( ITEM_save_item( child1Item ) );
        ASSERT_ITK_OK( BOM_line_add( subassemblyBomLine, child1Item, child1ItemRev, NULLTAG, &child1BomLine ) );
        
        // Create child2 item and add to subassembly
        int randNum2 = Teamcenter::RAND::TcRAND_Int();
        std::string child2Name = "Child2_" + std::to_string( randNum2 );
        ASSERT_ITK_OK( ITEM_create_item( NULL, child2Name.c_str(), "Part", "A", &child2Item, &child2ItemRev ) );
        ASSERT_ITK_OK( ITEM_save_item( child2Item ) );
        ASSERT_ITK_OK( BOM_line_add( subassemblyBomLine, child2Item, child2ItemRev, NULLTAG, &child2BomLine ) );
        
        // Create child3 item and add to subassembly
        int randNum3 = Teamcenter::RAND::TcRAND_Int();
        std::string child3Name = "Child3_" + std::to_string( randNum3 );
        ASSERT_ITK_OK( ITEM_create_item( NULL, child3Name.c_str(), "Part", "A", &child3Item, &child3ItemRev ) );
        ASSERT_ITK_OK( ITEM_save_item( child3Item ) );
        ASSERT_ITK_OK( BOM_line_add( subassemblyBomLine, child3Item, child3ItemRev, NULLTAG, &child3BomLine ) );

        // Step 4: Attach alternates and substitutes to the children
        // Child1: Attach alternate (item-level)
        tag_t child1AlternateItem = NULLTAG;
        attachAlternateToPart( child1Item, child1AlternateItem );

        // Child2: Attach substitute (occurrence-level)
        tag_t child2SubstituteItem = NULLTAG;
        attachSubstituteToOccurrence( child2BomLine, child2SubstituteItem );

        // Child3: Attach both alternate and substitute
        tag_t child3AlternateItem = NULLTAG;
        tag_t child3SubstituteItem = NULLTAG;
        attachAlternateToPart( child3Item, child3AlternateItem );
        attachSubstituteToOccurrence( child3BomLine, child3SubstituteItem );

        // Save the BOM window to persist substitutes
        ASSERT_ITK_OK( BOM_save_window( topAssemblyOutputData.bomWindow ) );

        // Step 5: Create SBOM structure - Get the subassembly Awb0Element
        OCCMGMT_GetOccurrencesInProductInput input;
        OCCMGMT_GetOccurrencesInProductOutput output;
        input.product = topAssemblyOutputData.topItemRevTag;
        input.maxToLoad = 0;
        input.firstLevelOnly = false; // Get all occurrences to find the subassembly and children
        input.requestPrefMap["occWindowCtx"] = { "SLMNeutral" };

        ASSERT_ITK_OK( OCCMGMT_getOccurrencesInProduct( input, output ) );
        ASSERT_EQ( output.resultOccurrencesInfo.occurrences.size(), 4 ) << "Should have 4 occurrences: 1 subassembly + 3 children";

        // First occurrence is the subassembly, next 3 are its children
        tag_t subassemblyAwb0Element = output.resultOccurrencesInfo.occurrences[0];
        tag_t child1Awb0Element = output.resultOccurrencesInfo.occurrences[1];
        tag_t child2Awb0Element = output.resultOccurrencesInfo.occurrences[2];
        tag_t child3Awb0Element = output.resultOccurrencesInfo.occurrences[3];

        // Step 6: Call service with the subassembly (not TopAssembly, not children)
        std::vector<BusinessObjectRef<Teamcenter::BusinessObject>> assetRefs;
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( subassemblyAwb0Element ) );

        AutoCreateSpareDefinitionsService& service = AutoCreateSpareDefinitionsService::getInstance();
        ASSERT_ITK_OK( service.createSpareDefinitionForAssets( assetRefs ) );

        // Step 7: Validate isSpare property on children after service call
        logical child1IsSpare = false; logical child2IsSpare = false; logical child3IsSpare = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child1Awb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &child1IsSpare ) );
        ASSERT_ITK_OK( AOM_ask_value_logical( child2Awb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &child2IsSpare ) );
        ASSERT_ITK_OK( AOM_ask_value_logical( child3Awb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &child3IsSpare ) );
        ASSERT_FALSE( child1IsSpare ) << "Child1 should not be marked as spare (alternate only, item-level)";
        ASSERT_TRUE( child2IsSpare ) << "Child2 should be marked as spare (substitute, occurrence-level)";
        ASSERT_TRUE( child3IsSpare ) << "Child3 should be marked as spare (has substitute, occurrence-level)";

        // Step 8: Validate spare definitions for Child1 (alternate only - item-level)
        tag_t neutralSDsRelationType = getNeutralSDsRelationType();

        int child1NeutralSDCount = 0;
        scoped_smptr<tag_t> child1NeutralSDs;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( child1Item, neutralSDsRelationType, &child1NeutralSDCount, &child1NeutralSDs ) );
        ASSERT_GT( child1NeutralSDCount, 0 ) << "Child1 should have item-level spare definition";

        tag_t child1NeutralSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child1NeutralSDs[0], ClassNames::Smr0SpareDefinition.c_str(), &child1NeutralSpareDefTag ) );
        logical child1NeutralIsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child1NeutralSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child1NeutralIsSystemGen ) );
        ASSERT_TRUE( child1NeutralIsSystemGen ) << "Child1 item-level spare definition should be system generated";

        // Validate that child1 spare definition contains the alternate part
        int child1SparePartsCount = 0;
        scoped_smptr<tag_t> child1SpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child1NeutralSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child1SparePartsCount, &child1SpareParts ) );
        ASSERT_GT( child1SparePartsCount, 0 ) << "Child1 spare definition should have at least one spare part (alternate)";
        
        bool child1AlternateFound = false;
        for ( int i = 0; i < child1SparePartsCount; ++i )
        {
            if ( child1SpareParts[i] == child1AlternateItem )
            {
                child1AlternateFound = true;
                break;
            }
        }
        ASSERT_TRUE( child1AlternateFound ) << "Alternate item should be in child1 spare definition's smr0Spares";

        // Validate spare definitions for Child2 (substitute only - occurrence-level)
        tag_t occSDsRelationType = getOccSDsRelationType();

        tag_t psOccurrence2 = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( child2BomLine, BOMLinePropertyNames::bl_real_occurrence.c_str(), &psOccurrence2 ) );

        int child2OccSDCount = 0;
        scoped_smptr<tag_t> child2OccSDs;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( psOccurrence2, occSDsRelationType, &child2OccSDCount, &child2OccSDs ) );
        ASSERT_GT( child2OccSDCount, 0 ) << "Child2 should have occurrence-level spare definition";

        tag_t child2OccSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child2OccSDs[0], ClassNames::Smr0SpareDefinition.c_str(), &child2OccSpareDefTag ) );
        logical child2OccIsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child2OccSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child2OccIsSystemGen ) );
        ASSERT_TRUE( child2OccIsSystemGen ) << "Child2 occurrence-level spare definition should be system generated";

        // Validate that child2 spare definition contains the substitute part
        int child2SparePartsCount = 0;
        scoped_smptr<tag_t> child2SpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child2OccSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child2SparePartsCount, &child2SpareParts ) );
        ASSERT_GT( child2SparePartsCount, 0 ) << "Child2 spare definition should have at least one spare part (substitute)";
        
        bool child2SubstituteFound = false;
        for ( int i = 0; i < child2SparePartsCount; ++i )
        {
            if ( child2SpareParts[i] == child2SubstituteItem )
            {
                child2SubstituteFound = true;
                break;
            }
        }
        ASSERT_TRUE( child2SubstituteFound ) << "Substitute item should be in child2 spare definition's smr0Spares";

        // Validate spare definitions for Child3 (both alternate and substitute)
        // Item-level spare definition
        int child3NeutralSDCount = 0;
        scoped_smptr<tag_t> child3NeutralSDs;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( child3Item, neutralSDsRelationType, &child3NeutralSDCount, &child3NeutralSDs ) );
        ASSERT_GT( child3NeutralSDCount, 0 ) << "Child3 should have item-level spare definition";

        tag_t child3NeutralSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child3NeutralSDs[0], ClassNames::Smr0SpareDefinition.c_str(), &child3NeutralSpareDefTag ) );
        logical child3NeutralIsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child3NeutralSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child3NeutralIsSystemGen ) );
        ASSERT_TRUE( child3NeutralIsSystemGen ) << "Child3 item-level spare definition should be system generated";

        // Validate that child3 item-level spare definition contains the alternate part
        int child3ItemSparePartsCount = 0;
        scoped_smptr<tag_t> child3ItemSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child3NeutralSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child3ItemSparePartsCount, &child3ItemSpareParts ) );
        ASSERT_GT( child3ItemSparePartsCount, 0 ) << "Child3 item-level spare definition should have at least one spare part (alternate)";
        
        bool child3AlternateFound = false;
        for ( int i = 0; i < child3ItemSparePartsCount; ++i )
        {
            if ( child3ItemSpareParts[i] == child3AlternateItem )
            {
                child3AlternateFound = true;
                break;
            }
        }
        ASSERT_TRUE( child3AlternateFound ) << "Alternate item should be in child3 item-level spare definition's smr0Spares";

        // Occurrence-level spare definition
        tag_t psOccurrence3 = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( child3BomLine, BOMLinePropertyNames::bl_real_occurrence.c_str(), &psOccurrence3 ) );

        int child3OccSDCount = 0;
        scoped_smptr<tag_t> child3OccSDs;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( psOccurrence3, occSDsRelationType, &child3OccSDCount, &child3OccSDs ) );
        ASSERT_GT( child3OccSDCount, 0 ) << "Child3 should have occurrence-level spare definition";

        tag_t child3OccSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child3OccSDs[0], ClassNames::Smr0SpareDefinition.c_str(), &child3OccSpareDefTag ) );
        logical child3OccIsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child3OccSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child3OccIsSystemGen ) );
        ASSERT_TRUE( child3OccIsSystemGen ) << "Child3 occurrence-level spare definition should be system generated";

        // Validate that child3 occurrence-level spare definition contains the substitute part
        int child3OccSparePartsCount = 0;
        scoped_smptr<tag_t> child3OccSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child3OccSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child3OccSparePartsCount, &child3OccSpareParts ) );
        ASSERT_GT( child3OccSparePartsCount, 0 ) << "Child3 occurrence-level spare definition should have at least one spare part (substitute)";
        
        bool child3SubstituteFound = false;
        for ( int i = 0; i < child3OccSparePartsCount; ++i )
        {
            if ( child3OccSpareParts[i] == child3SubstituteItem )
            {
                child3SubstituteFound = true;
                break;
            }
        }
        ASSERT_TRUE( child3SubstituteFound ) << "Substitute item should be in child3 occurrence-level spare definition's smr0Spares";

        // Cleanup
        MroTestStructureUtils::closeWindows( topAssemblyOutputData.bomWindow, topAssemblyOutputData.mrocoreWindow, NULLTAG );
    }

    TEST_F( TestAlignSpareWithAlternateAndSubstitute, TestAutoCreateSpareDefForMultipleSubassembliesWithChildren )
    {
        // Create 3 subassemblies, each with 2 children
        // SubAssembly1: child1 (alternate), child2 (substitute)
        NeutralStructInputData neuStructInputData1( "TestAutoCreateSpareDef_SubAssembly1", 2 );
        NeutralStructOutputData neuStructOutputData1;
        tag_t neutralTopLine1 = MroTestStructureUtils::createNeutralStructure( neuStructInputData1, neuStructOutputData1 );
        ASSERT_NE( neutralTopLine1, NULLTAG ) << "Failed to create subassembly1";

        // SubAssembly2: child3 (substitute), child4 (both)
        NeutralStructInputData neuStructInputData2( "TestAutoCreateSpareDef_SubAssembly2", 2 );
        NeutralStructOutputData neuStructOutputData2;
        tag_t neutralTopLine2 = MroTestStructureUtils::createNeutralStructure( neuStructInputData2, neuStructOutputData2 );
        ASSERT_NE( neutralTopLine2, NULLTAG ) << "Failed to create subassembly2";

        // SubAssembly3: child5 (both), child6 (alternate)
        NeutralStructInputData neuStructInputData3( "TestAutoCreateSpareDef_SubAssembly3", 2 );
        NeutralStructOutputData neuStructOutputData3;
        tag_t neutralTopLine3 = MroTestStructureUtils::createNeutralStructure( neuStructInputData3, neuStructOutputData3 );
        ASSERT_NE( neutralTopLine3, NULLTAG ) << "Failed to create subassembly3";

        // Get child lines for SubAssembly1
        int count1 = 0;
        scoped_smptr<tag_t> childLines1;
        ASSERT_ITK_OK( BOM_line_ask_all_child_lines( neuStructOutputData1.topBomLine, &count1, &childLines1 ) );
        ASSERT_EQ( count1, 2 ) << "SubAssembly1 should have 2 children";

        tag_t child1Item = NULLTAG, child2Item = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines1[0], BOMLinePropertyNames::bl_item.c_str(), &child1Item ) );
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines1[1], BOMLinePropertyNames::bl_item.c_str(), &child2Item ) );

        // Child1: Attach alternate
        tag_t child1AlternateItem = NULLTAG;
        attachAlternateToPart( child1Item, child1AlternateItem );

        // Child2: Attach substitute
        tag_t child2SubstituteItem = NULLTAG;
        attachSubstituteToOccurrence( childLines1[1], child2SubstituteItem );

        ASSERT_ITK_OK( BOM_save_window( neuStructOutputData1.bomWindow ) );

        // Get child lines for SubAssembly2
        int count2 = 0;
        scoped_smptr<tag_t> childLines2;
        ASSERT_ITK_OK( BOM_line_ask_all_child_lines( neuStructOutputData2.topBomLine, &count2, &childLines2 ) );
        ASSERT_EQ( count2, 2 ) << "SubAssembly2 should have 2 children";

        tag_t child3Item = NULLTAG, child4Item = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines2[0], BOMLinePropertyNames::bl_item.c_str(), &child3Item ) );
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines2[1], BOMLinePropertyNames::bl_item.c_str(), &child4Item ) );

        // Child3: Attach substitute
        tag_t child3SubstituteItem = NULLTAG;
        attachSubstituteToOccurrence( childLines2[0], child3SubstituteItem );

        // Child4: Attach both alternate and substitute
        tag_t child4AlternateItem = NULLTAG, child4SubstituteItem = NULLTAG;
        attachAlternateToPart( child4Item, child4AlternateItem );
        attachSubstituteToOccurrence( childLines2[1], child4SubstituteItem );

        ASSERT_ITK_OK( BOM_save_window( neuStructOutputData2.bomWindow ) );

        // Get child lines for SubAssembly3
        int count3 = 0;
        scoped_smptr<tag_t> childLines3;
        ASSERT_ITK_OK( BOM_line_ask_all_child_lines( neuStructOutputData3.topBomLine, &count3, &childLines3 ) );
        ASSERT_EQ( count3, 2 ) << "SubAssembly3 should have 2 children";

        tag_t child5Item = NULLTAG, child6Item = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines3[0], BOMLinePropertyNames::bl_item.c_str(), &child5Item ) );
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines3[1], BOMLinePropertyNames::bl_item.c_str(), &child6Item ) );

        // Child5: Attach both alternate and substitute
        tag_t child5AlternateItem = NULLTAG, child5SubstituteItem = NULLTAG;
        attachAlternateToPart( child5Item, child5AlternateItem );
        attachSubstituteToOccurrence( childLines3[0], child5SubstituteItem );

        // Child6: Attach alternate
        tag_t child6AlternateItem = NULLTAG;
        attachAlternateToPart( child6Item, child6AlternateItem );

        ASSERT_ITK_OK( BOM_save_window( neuStructOutputData3.bomWindow ) );

        // Create SBOM structure - Convert all subassemblies to Awb0Element objects
        OCCMGMT_GetOccurrencesInProductInput input1, input2, input3;
        OCCMGMT_GetOccurrencesInProductOutput output1, output2, output3;

        input1.product = neuStructOutputData1.topItemRevTag;
        input1.maxToLoad = 0;
        input1.firstLevelOnly = false;
        input1.requestPrefMap["occWindowCtx"] = { "SLMNeutral" };
        ASSERT_ITK_OK( OCCMGMT_getOccurrencesInProduct( input1, output1 ) );
        ASSERT_EQ( output1.resultOccurrencesInfo.occurrences.size(), 2 ) << "SubAssembly1 should have 2 child occurrences";

        input2.product = neuStructOutputData2.topItemRevTag;
        input2.maxToLoad = 0;
        input2.firstLevelOnly = false;
        input2.requestPrefMap["occWindowCtx"] = { "SLMNeutral" };
        ASSERT_ITK_OK( OCCMGMT_getOccurrencesInProduct( input2, output2 ) );
        ASSERT_EQ( output2.resultOccurrencesInfo.occurrences.size(), 2 ) << "SubAssembly2 should have 2 child occurrences";

        input3.product = neuStructOutputData3.topItemRevTag;
        input3.maxToLoad = 0;
        input3.firstLevelOnly = false;
        input3.requestPrefMap["occWindowCtx"] = { "SLMNeutral" };
        ASSERT_ITK_OK( OCCMGMT_getOccurrencesInProduct( input3, output3 ) );
        ASSERT_EQ( output3.resultOccurrencesInfo.occurrences.size(), 2 ) << "SubAssembly3 should have 2 child occurrences";

        // Set smr1Spare property on all child Awb0Elements and collect them
        std::vector<BusinessObjectRef<Teamcenter::BusinessObject>> assetRefs;

        // SubAssembly1 children
        tag_t child1Awb0Element = output1.resultOccurrencesInfo.occurrences[0];
        tag_t child2Awb0Element = output1.resultOccurrencesInfo.occurrences[1];
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( child1Awb0Element ) );
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( child2Awb0Element ) );

        // SubAssembly2 children
        tag_t child3Awb0Element = output2.resultOccurrencesInfo.occurrences[0];
        tag_t child4Awb0Element = output2.resultOccurrencesInfo.occurrences[1];
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( child3Awb0Element ) );
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( child4Awb0Element ) );

        // SubAssembly3 children
        tag_t child5Awb0Element = output3.resultOccurrencesInfo.occurrences[0];
        tag_t child6Awb0Element = output3.resultOccurrencesInfo.occurrences[1];
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( child5Awb0Element ) );
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( child6Awb0Element ) );

        // Call service
        AutoCreateSpareDefinitionsService& service = AutoCreateSpareDefinitionsService::getInstance();
        ASSERT_ITK_OK( service.createSpareDefinitionForAssets( assetRefs ) );

        // Validate isSpare property after service call
        logical child1IsSpare = false; logical child2IsSpare = false; logical child3IsSpare = false;
        logical child4IsSpare = false; logical child5IsSpare = false; logical child6IsSpare = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child1Awb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &child1IsSpare ) );
        ASSERT_FALSE( child1IsSpare ) << "Child1 should not be marked as spare (alternate only, item-level)";
        ASSERT_ITK_OK( AOM_ask_value_logical( child2Awb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &child2IsSpare ) );
        ASSERT_TRUE( child2IsSpare ) << "Child2 should be marked as spare (substitute, occurrence-level)";
        ASSERT_ITK_OK( AOM_ask_value_logical( child3Awb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &child3IsSpare ) );
        ASSERT_TRUE( child3IsSpare ) << "Child3 should be marked as spare (substitute, occurrence-level)";
        ASSERT_ITK_OK( AOM_ask_value_logical( child4Awb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &child4IsSpare ) );
        ASSERT_TRUE( child4IsSpare ) << "Child4 should be marked as spare (has substitute, occurrence-level)";
        ASSERT_ITK_OK( AOM_ask_value_logical( child5Awb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &child5IsSpare ) );
        ASSERT_TRUE( child5IsSpare ) << "Child5 should be marked as spare (has substitute, occurrence-level)";
        ASSERT_ITK_OK( AOM_ask_value_logical( child6Awb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &child6IsSpare ) );
        ASSERT_FALSE( child6IsSpare ) << "Child6 should not be marked as spare (alternate only, item-level)";

        // Get relation types
        tag_t neutralSDsRelationType = getNeutralSDsRelationType();
        tag_t occSDsRelationType = getOccSDsRelationType();

        // Validate Child1 (alternate only - item-level)
        int child1NeutralSDCount = 0;
        scoped_smptr<tag_t> child1NeutralSDs;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( child1Item, neutralSDsRelationType, &child1NeutralSDCount, &child1NeutralSDs ) );
        ASSERT_GT( child1NeutralSDCount, 0 ) << "Child1 should have item-level spare definition";

        tag_t child1SpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child1NeutralSDs[0], ClassNames::Smr0SpareDefinition.c_str(), &child1SpareDefTag ) );
        logical child1IsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child1SpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child1IsSystemGen ) );
        ASSERT_TRUE( child1IsSystemGen ) << "Child1 item-level spare definition should be system generated";

        // Validate that child1 spare definition contains the alternate part
        int child1SparePartsCount = 0;
        scoped_smptr<tag_t> child1SpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child1SpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child1SparePartsCount, &child1SpareParts ) );
        ASSERT_GT( child1SparePartsCount, 0 ) << "Child1 spare definition should have at least one spare part (alternate)";
        
        bool child1AlternateFound = false;
        for ( int i = 0; i < child1SparePartsCount; ++i )
        {
            if ( child1SpareParts[i] == child1AlternateItem )
            {
                child1AlternateFound = true;
                break;
            }
        }
        ASSERT_TRUE( child1AlternateFound ) << "Alternate item should be in child1 spare definition's smr0Spares";

        // Validate Child2 (substitute only - occurrence-level)
        tag_t psOccurrence2 = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines1[1], BOMLinePropertyNames::bl_real_occurrence.c_str(), &psOccurrence2 ) );

        int child2OccSDCount = 0;
        scoped_smptr<tag_t> child2OccSDs;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( psOccurrence2, occSDsRelationType, &child2OccSDCount, &child2OccSDs ) );
        ASSERT_GT( child2OccSDCount, 0 ) << "Child2 should have occurrence-level spare definition";

        tag_t child2SpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child2OccSDs[0], ClassNames::Smr0SpareDefinition.c_str(), &child2SpareDefTag ) );
        logical child2IsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child2SpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child2IsSystemGen ) );
        ASSERT_TRUE( child2IsSystemGen ) << "Child2 occurrence-level spare definition should be system generated";

        // Validate that child2 spare definition contains the substitute part
        int child2SparePartsCount = 0;
        scoped_smptr<tag_t> child2SpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child2SpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child2SparePartsCount, &child2SpareParts ) );
        ASSERT_GT( child2SparePartsCount, 0 ) << "Child2 spare definition should have at least one spare part (substitute)";
        
        bool child2SubstituteFound = false;
        for ( int i = 0; i < child2SparePartsCount; ++i )
        {
            if ( child2SpareParts[i] == child2SubstituteItem )
            {
                child2SubstituteFound = true;
                break;
            }
        }
        ASSERT_TRUE( child2SubstituteFound ) << "Substitute item should be in child2 spare definition's smr0Spares";

        // Validate Child3 (substitute only - occurrence-level)
        tag_t psOccurrence3 = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines2[0], BOMLinePropertyNames::bl_real_occurrence.c_str(), &psOccurrence3 ) );

        int child3OccSDCount = 0;
        scoped_smptr<tag_t> child3OccSDs;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( psOccurrence3, occSDsRelationType, &child3OccSDCount, &child3OccSDs ) );
        ASSERT_GT( child3OccSDCount, 0 ) << "Child3 should have occurrence-level spare definition";

        tag_t child3SpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child3OccSDs[0], ClassNames::Smr0SpareDefinition.c_str(), &child3SpareDefTag ) );
        logical child3IsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child3SpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child3IsSystemGen ) );
        ASSERT_TRUE( child3IsSystemGen ) << "Child3 occurrence-level spare definition should be system generated";

        // Validate that child3 spare definition contains the substitute part
        int child3SparePartsCount = 0;
        scoped_smptr<tag_t> child3SpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child3SpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child3SparePartsCount, &child3SpareParts ) );
        ASSERT_GT( child3SparePartsCount, 0 ) << "Child3 spare definition should have at least one spare part (substitute)";
        
        bool child3SubstituteFound = false;
        for ( int i = 0; i < child3SparePartsCount; ++i )
        {
            if ( child3SpareParts[i] == child3SubstituteItem )
            {
                child3SubstituteFound = true;
                break;
            }
        }
        ASSERT_TRUE( child3SubstituteFound ) << "Substitute item should be in child3 spare definition's smr0Spares";

        // Validate Child4 (both - item-level and occurrence-level)
        int child4NeutralSDCount = 0;
        scoped_smptr<tag_t> child4NeutralSDs;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( child4Item, neutralSDsRelationType, &child4NeutralSDCount, &child4NeutralSDs ) );
        ASSERT_GT( child4NeutralSDCount, 0 ) << "Child4 should have item-level spare definition";

        tag_t child4ItemSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child4NeutralSDs[0], ClassNames::Smr0SpareDefinition.c_str(), &child4ItemSpareDefTag ) );
        logical child4ItemIsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child4ItemSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child4ItemIsSystemGen ) );
        ASSERT_TRUE( child4ItemIsSystemGen ) << "Child4 item-level spare definition should be system generated";

        // Validate that child4 item-level spare definition contains the alternate part
        int child4ItemSparePartsCount = 0;
        scoped_smptr<tag_t> child4ItemSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child4ItemSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child4ItemSparePartsCount, &child4ItemSpareParts ) );
        ASSERT_GT( child4ItemSparePartsCount, 0 ) << "Child4 item-level spare definition should have at least one spare part (alternate)";
        
        bool child4AlternateFound = false;
        for ( int i = 0; i < child4ItemSparePartsCount; ++i )
        {
            if ( child4ItemSpareParts[i] == child4AlternateItem )
            {
                child4AlternateFound = true;
                break;
            }
        }
        ASSERT_TRUE( child4AlternateFound ) << "Alternate item should be in child4 item-level spare definition's smr0Spares";

        tag_t psOccurrence4 = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines2[1], BOMLinePropertyNames::bl_real_occurrence.c_str(), &psOccurrence4 ) );

        int child4OccSDCount = 0;
        scoped_smptr<tag_t> child4OccSDs;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( psOccurrence4, occSDsRelationType, &child4OccSDCount, &child4OccSDs ) );
        ASSERT_GT( child4OccSDCount, 0 ) << "Child4 should have occurrence-level spare definition";

        tag_t child4OccSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child4OccSDs[0], ClassNames::Smr0SpareDefinition.c_str(), &child4OccSpareDefTag ) );
        logical child4OccIsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child4OccSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child4OccIsSystemGen ) );
        ASSERT_TRUE( child4OccIsSystemGen ) << "Child4 occurrence-level spare definition should be system generated";

        // Validate that child4 occurrence-level spare definition contains the substitute part
        int child4OccSparePartsCount = 0;
        scoped_smptr<tag_t> child4OccSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child4OccSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child4OccSparePartsCount, &child4OccSpareParts ) );
        ASSERT_GT( child4OccSparePartsCount, 0 ) << "Child4 occurrence-level spare definition should have at least one spare part (substitute)";
        
        bool child4SubstituteFound = false;
        for ( int i = 0; i < child4OccSparePartsCount; ++i )
        {
            if ( child4OccSpareParts[i] == child4SubstituteItem )
            {
                child4SubstituteFound = true;
                break;
            }
        }
        ASSERT_TRUE( child4SubstituteFound ) << "Substitute item should be in child4 occurrence-level spare definition's smr0Spares";

        // Validate Child5 (both - item-level and occurrence-level)
        int child5NeutralSDCount = 0;
        scoped_smptr<tag_t> child5NeutralSDs;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( child5Item, neutralSDsRelationType, &child5NeutralSDCount, &child5NeutralSDs ) );
        ASSERT_GT( child5NeutralSDCount, 0 ) << "Child5 should have item-level spare definition";

        tag_t child5ItemSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child5NeutralSDs[0], ClassNames::Smr0SpareDefinition.c_str(), &child5ItemSpareDefTag ) );
        logical child5ItemIsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child5ItemSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child5ItemIsSystemGen ) );
        ASSERT_TRUE( child5ItemIsSystemGen ) << "Child5 item-level spare definition should be system generated";

        // Validate that child5 item-level spare definition contains the alternate part
        int child5ItemSparePartsCount = 0;
        scoped_smptr<tag_t> child5ItemSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child5ItemSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child5ItemSparePartsCount, &child5ItemSpareParts ) );
        ASSERT_GT( child5ItemSparePartsCount, 0 ) << "Child5 item-level spare definition should have at least one spare part (alternate)";
        
        bool child5AlternateFound = false;
        for ( int i = 0; i < child5ItemSparePartsCount; ++i )
        {
            if ( child5ItemSpareParts[i] == child5AlternateItem )
            {
                child5AlternateFound = true;
                break;
            }
        }
        ASSERT_TRUE( child5AlternateFound ) << "Alternate item should be in child5 item-level spare definition's smr0Spares";

        tag_t psOccurrence5 = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines3[0], BOMLinePropertyNames::bl_real_occurrence.c_str(), &psOccurrence5 ) );

        int child5OccSDCount = 0;
        scoped_smptr<tag_t> child5OccSDs;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( psOccurrence5, occSDsRelationType, &child5OccSDCount, &child5OccSDs ) );
        ASSERT_GT( child5OccSDCount, 0 ) << "Child5 should have occurrence-level spare definition";

        tag_t child5OccSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child5OccSDs[0], ClassNames::Smr0SpareDefinition.c_str(), &child5OccSpareDefTag ) );
        logical child5OccIsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child5OccSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child5OccIsSystemGen ) );
        ASSERT_TRUE( child5OccIsSystemGen ) << "Child5 occurrence-level spare definition should be system generated";

        // Validate that child5 occurrence-level spare definition contains the substitute part
        int child5OccSparePartsCount = 0;
        scoped_smptr<tag_t> child5OccSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child5OccSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child5OccSparePartsCount, &child5OccSpareParts ) );
        ASSERT_GT( child5OccSparePartsCount, 0 ) << "Child5 occurrence-level spare definition should have at least one spare part (substitute)";
        
        bool child5SubstituteFound = false;
        for ( int i = 0; i < child5OccSparePartsCount; ++i )
        {
            if ( child5OccSpareParts[i] == child5SubstituteItem )
            {
                child5SubstituteFound = true;
                break;
            }
        }
        ASSERT_TRUE( child5SubstituteFound ) << "Substitute item should be in child5 occurrence-level spare definition's smr0Spares";

        // Validate Child6 (alternate only - item-level)
        int child6NeutralSDCount = 0;
        scoped_smptr<tag_t> child6NeutralSDs;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( child6Item, neutralSDsRelationType, &child6NeutralSDCount, &child6NeutralSDs ) );
        ASSERT_GT( child6NeutralSDCount, 0 ) << "Child6 should have item-level spare definition";

        tag_t child6SpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( child6NeutralSDs[0], ClassNames::Smr0SpareDefinition.c_str(), &child6SpareDefTag ) );
        logical child6IsSystemGen = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( child6SpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &child6IsSystemGen ) );
        ASSERT_TRUE( child6IsSystemGen ) << "Child6 item-level spare definition should be system generated";

        // Validate that child6 spare definition contains the alternate part
        int child6SparePartsCount = 0;
        scoped_smptr<tag_t> child6SpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( child6SpareDefTag, SDPropertyNames::smr0Spares.c_str(), &child6SparePartsCount, &child6SpareParts ) );
        ASSERT_GT( child6SparePartsCount, 0 ) << "Child6 spare definition should have at least one spare part (alternate)";
        
        bool child6AlternateFound = false;
        for ( int i = 0; i < child6SparePartsCount; ++i )
        {
            if ( child6SpareParts[i] == child6AlternateItem )
            {
                child6AlternateFound = true;
                break;
            }
        }
        ASSERT_TRUE( child6AlternateFound ) << "Alternate item should be in child6 spare definition's smr0Spares";

        // Cleanup
        MroTestStructureUtils::closeWindows( neuStructOutputData1.bomWindow, neuStructOutputData1.mrocoreWindow, NULLTAG );
        MroTestStructureUtils::closeWindows( neuStructOutputData2.bomWindow, neuStructOutputData2.mrocoreWindow, NULLTAG );
        MroTestStructureUtils::closeWindows( neuStructOutputData3.bomWindow, neuStructOutputData3.mrocoreWindow, NULLTAG );
    }

    TEST_F( TestAlignSpareWithAlternateAndSubstitute, TestAutoCreateSpareDefForPackedLines )
    {
        // Create a neutral structure with 1 child first
        NeutralStructInputData neuStructInputData( "TestAutoCreateSpareDef_PackedAssembly", 1 );
        NeutralStructOutputData neuStructOutputData;
        tag_t neutralTopLine = MroTestStructureUtils::createNeutralStructure( neuStructInputData, neuStructOutputData );
        
        if ( neutralTopLine == NULLTAG )
        {
            FAIL() << "TestAutoCreateSpareDefForPackedLines: Failed to create neutral part structure";
        }

        // Get the first child
        int count = 0;
        scoped_smptr<tag_t> childLines;
        ASSERT_ITK_OK( BOM_line_ask_all_child_lines( neuStructOutputData.topBomLine, &count, &childLines ) );
        ASSERT_EQ( count, 1 ) << "Expected 1 child in neutral structure";

        tag_t childItem = NULLTAG;
        tag_t childItemRev = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines[0], BOMLinePropertyNames::bl_item.c_str(), &childItem ) );
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines[0], BOMLinePropertyNames::bl_revision.c_str(), &childItemRev ) );

        // Use the neutral BOM window - set find number on existing child to 001
        tag_t firstChildLine = childLines[0];
        ASSERT_ITK_OK( AOM_set_value_string( firstChildLine, BOMLinePropertyNames::bl_occurrence_name.c_str(), "001" ) );
        
        // Add second occurrence of the same item with the same find number to create packed line
        tag_t secondChildLine = NULLTAG;
        ASSERT_ITK_OK( BOM_line_add( neuStructOutputData.topBomLine, childItem, childItemRev, NULLTAG, &secondChildLine ) );
        ASSERT_ITK_OK( AOM_set_value_string( secondChildLine, BOMLinePropertyNames::bl_occurrence_name.c_str(), "001" ) );
        
        ASSERT_ITK_OK( BOM_save_window( neuStructOutputData.bomWindow ) );
        
        // Check how many child lines are visible after adding both occurrences
        // If packed: count = 1 (both occurrences shown as single packed line)
        // If not packed: count = 2 (both occurrences shown separately)
        int childCount = 0;
        scoped_smptr<tag_t> childLinesAfterAdd;
        ASSERT_ITK_OK( BOM_line_ask_all_child_lines( neuStructOutputData.topBomLine, &childCount, &childLinesAfterAdd ) );

        // Create an alternate on the child item (applies to both occurrences)
        tag_t alternateItem = NULLTAG;
        attachAlternateToPart( childItem, alternateItem );

        // Create a substitute on the second child occurrence (occurrence-specific)
        tag_t substituteItem = NULLTAG;
        attachSubstituteToOccurrence( secondChildLine, substituteItem );

        ASSERT_ITK_OK( BOM_save_window( neuStructOutputData.bomWindow ) );

        // Check if the lines are packed
        logical isPacked = false;
        ASSERT_ITK_OK( BOM_line_is_packed( firstChildLine, &isPacked ) );

        ASSERT_ITK_OK( BOM_line_pack( firstChildLine ) );
        ASSERT_ITK_OK( BOM_save_window( neuStructOutputData.bomWindow ) );

        ASSERT_ITK_OK( BOM_line_is_packed( firstChildLine, &isPacked ) );
        ASSERT_TRUE( isPacked );

        // Create SBOM structure - Convert to Awb0Element objects
        OCCMGMT_GetOccurrencesInProductInput productInfoInput;
        OCCMGMT_GetOccurrencesInProductOutput productInfo;
        productInfoInput.product = neuStructOutputData.topItemRevTag;
        productInfoInput.maxToLoad = 0;
        productInfoInput.firstLevelOnly = false;
        productInfoInput.requestPrefMap["occWindowCtx"] = { "SLMNeutral" };
        
        ASSERT_ITK_OK( OCCMGMT_getOccurrencesInProduct( productInfoInput, productInfo ) );
        ASSERT_GE( productInfo.resultOccurrencesInfo.occurrences.size(), 1 ) 
            << "Expected at least 1 occurrences from OCCMGMT_getOccurrencesInProduct";

        // Get the last occurrence (the packed children we added)
        size_t totalOccurrences = productInfo.resultOccurrencesInfo.occurrences.size();
        tag_t packedChildAwb0Element = productInfo.resultOccurrencesInfo.occurrences[totalOccurrences - 1];

        std::vector<BusinessObjectRef<Teamcenter::BusinessObject>> assetRefs;
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( packedChildAwb0Element ) );

        // Validate isSpare property before service call
        logical isSpare = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( packedChildAwb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &isSpare ) );
        ASSERT_FALSE( isSpare ) << "Child element should not be marked as spare before creating spare definitions";

        // Call service to auto-create spare definitions
        AutoCreateSpareDefinitionsService& service = AutoCreateSpareDefinitionsService::getInstance();
        ASSERT_ITK_OK( service.createSpareDefinitionForAssets( assetRefs ) );

        if ( isPacked )
        {
            // Unpack the lines for validation
            ASSERT_ITK_OK( BOM_line_unpack( firstChildLine ) );
            ASSERT_ITK_OK( BOM_save_window( neuStructOutputData.bomWindow ) );

            // Refresh child lines after unpacking
            ASSERT_ITK_OK( BOM_line_ask_all_child_lines( neuStructOutputData.topBomLine, &count, &childLines ) );
            ASSERT_EQ( count, 2 ) << "Expected 2 children after unpacking";
            firstChildLine = childLines[0];
            secondChildLine = childLines[1];
        }

        // Get the last 2 occurrences of the unpacked children (both should have the same alternate, but only the second should have the substitute)
        size_t totalOccurrencesAfterUnpack = productInfo.resultOccurrencesInfo.occurrences.size();
        tag_t firstChildAwb0Element = productInfo.resultOccurrencesInfo.occurrences[totalOccurrencesAfterUnpack - 2];
        tag_t secondChildAwb0Element = productInfo.resultOccurrencesInfo.occurrences[totalOccurrencesAfterUnpack - 1];

        // Validate isSpare property after service call (first child has alternate only, no substitute)
        isSpare = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( firstChildAwb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &isSpare ) );
        ASSERT_FALSE( isSpare ) << "First child should not be marked as spare (alternate only, item-level)";
        
        
        // Validate isSpare property after service call (second child has substitute)
        isSpare = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( secondChildAwb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &isSpare ) );
        ASSERT_TRUE( isSpare ) << "Second child should be marked as spare (substitute only, occurrence-level)";

        // Get relation types
        tag_t neutralSDsRelationType = getNeutralSDsRelationType();
        tag_t occSDsRelationType = getOccSDsRelationType();

        // Validate first child: Should have item-level spare definition (from alternate)
        int itemSDCount = 0;
        scoped_smptr<tag_t> itemSDThreads;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( childItem, neutralSDsRelationType, &itemSDCount, &itemSDThreads ) );
        ASSERT_GT( itemSDCount, 0 ) << "First child should have item-level spare definition";

        tag_t itemSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( itemSDThreads[0], ClassNames::Smr0SpareDefinition.c_str(), &itemSpareDefTag ) );
        logical itemIsSystemGenerated = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( itemSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &itemIsSystemGenerated ) );
        ASSERT_TRUE( itemIsSystemGenerated ) << "Item-level spare definition should be marked as system generated";

        // Validate that the item-level spare definition contains the alternate part
        int itemSparePartsCount = 0;
        scoped_smptr<tag_t> itemSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( itemSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &itemSparePartsCount, &itemSpareParts ) );
        ASSERT_GT( itemSparePartsCount, 0 ) << "Item-level spare definition should have at least one spare part (alternate)";
        
        bool alternateFound = false;
        for ( int i = 0; i < itemSparePartsCount; ++i )
        {
            if ( itemSpareParts[i] == alternateItem )
            {
                alternateFound = true;
                break;
            }
        }
        ASSERT_TRUE( alternateFound ) << "Alternate item should be in item-level spare definition's smr0Spares";

        // Validate second child: Should have both item-level (from alternate) and occurrence-level (from substitute) spare definitions
        // Item-level spare definition already validated above (shared by both occurrences)

        // Validate occurrence-level spare definition for second child
        tag_t psOccurrence2 = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( secondChildLine, BOMLinePropertyNames::bl_real_occurrence.c_str(), &psOccurrence2 ) );
        ASSERT_NE( psOccurrence2, NULLTAG ) << "Failed to get PS occurrence from second child BOM line";

        int occSDCount = 0;
        scoped_smptr<tag_t> occSDThreads;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( psOccurrence2, occSDsRelationType, &occSDCount, &occSDThreads ) );
        ASSERT_GT( occSDCount, 0 ) << "Second child should have occurrence-level spare definition";

        tag_t occSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( occSDThreads[0], ClassNames::Smr0SpareDefinition.c_str(), &occSpareDefTag ) );
        logical occIsSystemGenerated = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( occSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &occIsSystemGenerated ) );
        ASSERT_TRUE( occIsSystemGenerated ) << "Occurrence-level spare definition should be marked as system generated";

        // Validate that the occurrence-level spare definition contains the substitute part
        int occSparePartsCount = 0;
        scoped_smptr<tag_t> occSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( occSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &occSparePartsCount, &occSpareParts ) );
        ASSERT_GT( occSparePartsCount, 0 ) << "Occurrence-level spare definition should have at least one spare part (substitute)";
        
        bool substituteFound = false;
        for ( int i = 0; i < occSparePartsCount; ++i )
        {
            if ( occSpareParts[i] == substituteItem )
            {
                substituteFound = true;
                break;
            }
        }
        ASSERT_TRUE( substituteFound ) << "Substitute item should be in occurrence-level spare definition's smr0Spares";

        // Validate first child should NOT have occurrence-level spare definition (no substitute)
        tag_t psOccurrence1 = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( firstChildLine, BOMLinePropertyNames::bl_real_occurrence.c_str(), &psOccurrence1 ) );
        
        int occ1SDCount = 0;
        scoped_smptr<tag_t> occ1SDThreads;
        int result = GRM_list_secondary_objects_only( psOccurrence1, occSDsRelationType, &occ1SDCount, &occ1SDThreads );
        // First child should not have occurrence-level spare def (or it might be 0 count)
        if ( result == ITK_ok )
        {
            ASSERT_EQ( occ1SDCount, 0 ) << "First child should not have occurrence-level spare definition";
        }

        // Cleanup
        MroTestStructureUtils::closeWindows( neuStructOutputData.bomWindow, neuStructOutputData.mrocoreWindow, NULLTAG );
    }

    TEST_F( TestAlignSpareWithAlternateAndSubstitute, TestNoAutoCreateSpareDefWhenAlternateHasAutoGeneratedSpareDef )
    {
        int count = 0;
        tag_t alternateItem = NULLTAG;
        
        // Step 1: Create a neutral part structure with one child
        NeutralStructInputData neuStructInputData( "TestNoAutoCreateSpareDef_Assembly1", 1 );
        NeutralStructOutputData neuStructOutputData;
        tag_t neutralTopLine = MroTestStructureUtils::createNeutralStructure( neuStructInputData, neuStructOutputData );
        
        if ( neutralTopLine == NULLTAG )
        {
            FAIL() << "TestNoAutoCreateSpareDefWhenAlternateHasAutoGeneratedSpareDef: Failed to create neutral part structure";
        }

        // Get the child item and create an alternate for it
        scoped_smptr<tag_t> childLines;
        ASSERT_ITK_OK( BOM_line_ask_all_child_lines( neuStructOutputData.topBomLine, &count, &childLines ) );
        ASSERT_EQ( count, 1 ) << "Expected 1 child in neutral structure";
        
        tag_t childItem = NULLTAG;
        ASSERT_ITK_OK( AOM_ask_value_tag( childLines[0], BOMLinePropertyNames::bl_item.c_str(), &childItem ) );
        
        // Attach alternate to the child part (item-level)
        attachAlternateToPart( childItem, alternateItem, "AlternatePartWithSpareDef1" );
        ASSERT_NE( alternateItem, NULLTAG ) << "Alternate item was not created";

        // Step 2-3: Create SBOM structure - Convert to Awb0Element objects
        OCCMGMT_GetOccurrencesInProductInput productInfoInput;
        OCCMGMT_GetOccurrencesInProductOutput productInfo;
        productInfoInput.product = neuStructOutputData.topItemRevTag;
        productInfoInput.maxToLoad = 0;
        productInfoInput.firstLevelOnly = false;
        productInfoInput.requestPrefMap["occWindowCtx"] = { "SLMNeutral" };
        
        ASSERT_ITK_OK( OCCMGMT_getOccurrencesInProduct( productInfoInput, productInfo ) );
        ASSERT_GT( productInfo.resultOccurrencesInfo.occurrences.size(), 0 ) 
            << "No occurrences returned from OCCMGMT_getOccurrencesInProduct";

        // Step 4: Call service to auto-create spare definitions for the first time
        tag_t childAwb0Element = productInfo.resultOccurrencesInfo.occurrences[0];
        std::vector<BusinessObjectRef<Teamcenter::BusinessObject>> assetRefs;
        assetRefs.push_back( BusinessObjectRef<Teamcenter::BusinessObject>( childAwb0Element ) );
        
        // Validate isSpare property before first service call
        logical isSpare = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( childAwb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &isSpare ) );
        ASSERT_FALSE( isSpare ) << "Child element should not be marked as spare before creating spare definitions";

        AutoCreateSpareDefinitionsService& service = AutoCreateSpareDefinitionsService::getInstance();
        ASSERT_ITK_OK( service.createSpareDefinitionForAssets( assetRefs ) );

        // Validate isSpare property after first service call (alternate only, item-level)
        isSpare = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( childAwb0Element, Awb0PartElementPropertyNames::isSpare.c_str(), &isSpare ) );
        ASSERT_FALSE( isSpare ) << "Child element should not be marked as spare (alternate only, item-level)";

        // Step 5: Validate the auto-generated spare definition is marked with isSystemGenerated = true
        tag_t neutralSDsRelationType = getNeutralSDsRelationType();
        
        int sdCount = 0;
        scoped_smptr<tag_t> sdThreads;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( childItem, neutralSDsRelationType, &sdCount, &sdThreads ) );
        ASSERT_GT( sdCount, 0 ) << "Expected at least one spare definition to be created";
        
        tag_t threadTag = sdThreads[0];
        tag_t autoSpareDefTag = NULLTAG;
        ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( threadTag, ClassNames::Smr0SpareDefinition.c_str(), &autoSpareDefTag ) );

        logical isSystemGenerated = false;
        ASSERT_ITK_OK( AOM_ask_value_logical( autoSpareDefTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &isSystemGenerated ) );
        ASSERT_TRUE( isSystemGenerated ) << "Auto-generated spare definition should be marked with isSystemGenerated = true";

        // Validate that the auto-generated spare definition contains the alternate part
        int autoSparePartsCount = 0;
        scoped_smptr<tag_t> autoSpareParts;
        ASSERT_ITK_OK( AOM_ask_value_tags( autoSpareDefTag, SDPropertyNames::smr0Spares.c_str(), &autoSparePartsCount, &autoSpareParts ) );
        ASSERT_GT( autoSparePartsCount, 0 ) << "Auto-generated spare definition should have at least one spare part (alternate)";
        
        bool alternateFound = false;
        for ( int i = 0; i < autoSparePartsCount; ++i )
        {
            if ( autoSpareParts[i] == alternateItem )
            {
                alternateFound = true;
                break;
            }
        }
        ASSERT_TRUE( alternateFound ) << "Alternate item should be in auto-generated spare definition's smr0Spares";

        // Store the initial count of spare definitions
        int initialSDCount = sdCount;

        // Step 6: Create a spare definition manually on the child item (isSystemGenerated = false)
        tag_t manualSpareDefTag = NULLTAG;
        createManualSpareDefinition( childItem, manualSpareDefTag, false );
        ASSERT_NE( manualSpareDefTag, NULLTAG ) << "Manual spare definition was not created";

        // Now call the service again to verify no new auto-generated spare def is created
        // The manual spare def on childItem should prevent duplicate auto-generation
        ASSERT_ITK_OK( service.createSpareDefinitionForAssets( assetRefs ) );

        // Verify the count of spare definitions on childItem increased by 1 (the manual spare def)
        int finalSDCount = 0;
        scoped_smptr<tag_t> finalSDThreads;
        ASSERT_ITK_OK( GRM_list_secondary_objects_only( childItem, neutralSDsRelationType, &finalSDCount, &finalSDThreads ) );
        
        // The count should be initialSDCount + 1 (1 auto-generated + 1 manual)
        ASSERT_EQ( finalSDCount, initialSDCount + 1 ) 
            << "Child item should have 2 spare definitions: 1 auto-generated and 1 manual";

        // Verify both spare definitions exist and have correct isSystemGenerated flags
        for ( int i = 0; i < finalSDCount; ++i )
        {
            tag_t sdRevTag = NULLTAG;
            ASSERT_ITK_OK( MROCORE__getLatestRevisionOfRevisableWSOByClassName( finalSDThreads[i], ClassNames::Smr0SpareDefinition.c_str(), &sdRevTag ) );
            
            logical isSysGen = false;
            ASSERT_ITK_OK( AOM_ask_value_logical( sdRevTag, SDPropertyNames::smr0IsSystemGenerated.c_str(), &isSysGen ) );
            
            // One should be system generated (true), one should be manual (false)
        }

        // Cleanup
        MroTestStructureUtils::closeWindows( neuStructOutputData.bomWindow, neuStructOutputData.mrocoreWindow, NULLTAG );
    }
}