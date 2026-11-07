
#include "libcockatrice/filters/filter_tree.h"

#include <gtest/gtest.h>
#include <libcockatrice/card/card_localization.h>
#include <libcockatrice/filters/filter_string.h>

namespace
{
class FilterTreeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        language = CardSearchLanguage{};

        island = CardInfo::newInstance("Island", "", false, {{"colors", "N"}, {"coloridentity", "S"}}, {}, {}, {}, {});
        artifact =
            CardInfo::newInstance("Artifact", "", false, {{"colors", "N"}, {"coloridentity", "N"}}, {}, {}, {}, {});
        truth = CardInfo::newInstance("Truth", "", false, {{"colors", "S"}, {"coloridentity", "S"}}, {}, {}, {}, {});
        notDead =
            CardInfo::newInstance("Not Dead", "", false, {{"colors", "O"}, {"coloridentity", "O"}}, {}, {}, {}, {});
        wallOfDenial = CardInfo::newInstance("Wall of Denial", "", false, {{"colors", "MS"}, {"coloridentity", "MS"}},
                                             {}, {}, {}, {});
        maelstromAngel = CardInfo::newInstance("Maelstrom Archangel", "", false,
                                               {{"colors", "MSOGE"}, {"coloridentity", "MSOGE"}}, {}, {}, {}, {});
    }

    CardSearchLanguage language;

    CardData island;         // N/S
    CardData artifact;       // N
    CardData truth;          // S
    CardData notDead;        // O
    CardData wallOfDenial;   // MS
    CardData maelstromAngel; // MSOGE
};
} // namespace

// =====================================================================================================================
// acceptColor
// =====================================================================================================================

TEST_F(FilterTreeTest, Color)
{
    FilterTree filterTree;
    filterTree.termNode(CardFilter::Attr::AttrColor, CardFilter::Type::TypeAnd, "ms");
    ASSERT_FALSE(filterTree.acceptsCard(island, language));
    ASSERT_FALSE(filterTree.acceptsCard(artifact, language));
    ASSERT_FALSE(filterTree.acceptsCard(truth, language));
    ASSERT_FALSE(filterTree.acceptsCard(notDead, language));
    ASSERT_TRUE(filterTree.acceptsCard(wallOfDenial, language));
    ASSERT_TRUE(filterTree.acceptsCard(maelstromAngel, language));
}

TEST_F(FilterTreeTest, ColorColorless)
{
    FilterTree filterTree;
    filterTree.termNode(CardFilter::Attr::AttrColor, CardFilter::Type::TypeAnd, "N");
    ASSERT_TRUE(filterTree.acceptsCard(island, language));
    ASSERT_TRUE(filterTree.acceptsCard(artifact, language));
    ASSERT_FALSE(filterTree.acceptsCard(truth, language));
    ASSERT_FALSE(filterTree.acceptsCard(notDead, language));
    ASSERT_FALSE(filterTree.acceptsCard(wallOfDenial, language));
    ASSERT_FALSE(filterTree.acceptsCard(maelstromAngel, language));
}

TEST_F(FilterTreeTest, ColorNot)
{
    FilterTree filterTree;
    filterTree.termNode(CardFilter::Attr::AttrColor, CardFilter::Type::TypeAndNot, "o");
    ASSERT_TRUE(filterTree.acceptsCard(island, language));
    ASSERT_TRUE(filterTree.acceptsCard(artifact, language));
    ASSERT_TRUE(filterTree.acceptsCard(truth, language));
    ASSERT_FALSE(filterTree.acceptsCard(notDead, language));
    ASSERT_TRUE(filterTree.acceptsCard(wallOfDenial, language));
    ASSERT_FALSE(filterTree.acceptsCard(maelstromAngel, language));
}

// =====================================================================================================================
// acceptColorIdentity
// =====================================================================================================================

TEST_F(FilterTreeTest, ColorIdentity)
{
    FilterTree filterTree;
    filterTree.termNode(CardFilter::Attr::AttrColorIdentity, CardFilter::Type::TypeAnd, "ms");
    ASSERT_FALSE(filterTree.acceptsCard(island, language));
    ASSERT_FALSE(filterTree.acceptsCard(artifact, language));
    ASSERT_FALSE(filterTree.acceptsCard(truth, language));
    ASSERT_FALSE(filterTree.acceptsCard(notDead, language));
    ASSERT_TRUE(filterTree.acceptsCard(wallOfDenial, language));
    ASSERT_TRUE(filterTree.acceptsCard(maelstromAngel, language));
}

TEST_F(FilterTreeTest, ColorIdentityColorless)
{
    FilterTree filterTree;
    filterTree.termNode(CardFilter::Attr::AttrColorIdentity, CardFilter::Type::TypeAnd, "N");
    ASSERT_FALSE(filterTree.acceptsCard(island, language));
    ASSERT_TRUE(filterTree.acceptsCard(artifact, language));
    ASSERT_FALSE(filterTree.acceptsCard(truth, language));
    ASSERT_FALSE(filterTree.acceptsCard(notDead, language));
    ASSERT_FALSE(filterTree.acceptsCard(wallOfDenial, language));
    ASSERT_FALSE(filterTree.acceptsCard(maelstromAngel, language));
}

TEST_F(FilterTreeTest, ColorIdentityNot)
{
    FilterTree filterTree;
    filterTree.termNode(CardFilter::Attr::AttrColorIdentity, CardFilter::Type::TypeAndNot, "o");
    ASSERT_TRUE(filterTree.acceptsCard(island, language));
    ASSERT_TRUE(filterTree.acceptsCard(artifact, language));
    ASSERT_TRUE(filterTree.acceptsCard(truth, language));
    ASSERT_FALSE(filterTree.acceptsCard(notDead, language));
    ASSERT_TRUE(filterTree.acceptsCard(wallOfDenial, language));
    ASSERT_FALSE(filterTree.acceptsCard(maelstromAngel, language));
}

TEST_F(FilterTreeTest, ColorIdentityMultiple)
{
    // Simulate a card whose identity allows M and S.
    FilterTree filterTree;
    filterTree.termNode(CardFilter::Attr::AttrColorIdentity, CardFilter::Type::TypeAndNot, "o");
    filterTree.termNode(CardFilter::Attr::AttrColorIdentity, CardFilter::Type::TypeAndNot, "g");
    filterTree.termNode(CardFilter::Attr::AttrColorIdentity, CardFilter::Type::TypeAndNot, "e");
    ASSERT_TRUE(filterTree.acceptsCard(island, language));
    ASSERT_TRUE(filterTree.acceptsCard(artifact, language));
    ASSERT_TRUE(filterTree.acceptsCard(truth, language));
    ASSERT_FALSE(filterTree.acceptsCard(notDead, language));
    ASSERT_TRUE(filterTree.acceptsCard(wallOfDenial, language));
    ASSERT_FALSE(filterTree.acceptsCard(maelstromAngel, language));
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
