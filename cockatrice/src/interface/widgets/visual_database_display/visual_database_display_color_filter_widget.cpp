#include "visual_database_display_color_filter_widget.h"

#include "../cards/additional_info/essence_filter_picker.h"
#include "filter_tree_model.h"
#include "libcockatrice/filters/filter_card.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QSharedPointer>
#include <QSizePolicy>
#include <QString>
#include <QVariant>
#include <libcockatrice/card/essence_types.h>
#include <libcockatrice/filters/filter_tree.h>

namespace
{

/** @brief Remove the filters that apply to the given color and attribute from the given model. */
static void removeFilters(FilterTreeModel *filterModel, QChar color, CardFilter::Attr attr)
{
    for (const CardFilter *filter : filterModel->getFiltersOfType(attr)) {
        if (filter->term() == color) {
            filterModel->removeFilter(filter);
        }
    }
}
} // namespace

/**
 * This widget provides a graphical control element for the CardFilter::Attr::AttrColor and
 * CardFilter::Attr::AttrColorIdentity filters applied to the filter model.
 * @param parent The Qt Widget that this widget will parent to
 * @param _filterModel The filter model that this widget will manipulate
 */
VisualDatabaseDisplayColorFilterWidget::VisualDatabaseDisplayColorFilterWidget(QWidget *parent,
                                                                               FilterTreeModel *_filterModel)
    : QWidget(parent), filterModel(_filterModel), layout(new QHBoxLayout(this))
{
    setLayout(layout);
    layout->setSpacing(3);
    layout->setContentsMargins(0, 0, 0, 0);

    // Share the same compact Essence selection UI as Visual Deck Storage.
    // A 15-icon horizontal row cannot fit the database filter toolbar.
    essencePicker = new EssenceFilterPicker(this);
    layout->addWidget(essencePicker);
    // FlowWidget measures THIS parent widget, not its nested button. Give it
    // an explicit size hint that cannot clip the picker.
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    setMinimumWidth(essencePicker->width());

    connect(essencePicker, &EssenceFilterPicker::essenceToggled, this,
            &VisualDatabaseDisplayColorFilterWidget::handleColorToggled);

    connect(essencePicker, &EssenceFilterPicker::selectionCleared, this, [this]() {
        filterModel->clearFiltersOfType(CardFilter::Attr::AttrColor);
        filterModel->clearFiltersOfType(CardFilter::Attr::AttrColorIdentity);
    });

    modeComboBox = new QComboBox(this);
    essencePicker->addPopupControl(modeComboBox);

    connect(modeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &VisualDatabaseDisplayColorFilterWidget::updateFilterMode);

    connect(filterModel, &FilterTreeModel::layoutChanged, this,
            [this]() { syncDebouncer.call([this] { syncWithFilterModel(); }); });

    retranslateUi();
    syncWithFilterModel();
}

void VisualDatabaseDisplayColorFilterWidget::retranslateUi()
{
    essencePicker->retranslateUi();

    modeComboBox->blockSignals(true);
    modeComboBox->clear();
    modeComboBox->addItem(tr("Includes"), QVariant::fromValue(FilterMode::Includes));
    modeComboBox->addItem(tr("Excludes"), QVariant::fromValue(FilterMode::Excludes));
    modeComboBox->addItem(tr("Include / Exclude"), QVariant::fromValue(FilterMode::IncludeExclude));
    modeComboBox->setToolTip(tr("How selected and unselected Essence types are combined in the filter"));

    modeComboBox->addItem(tr("Exact match"), QVariant::fromValue(FilterMode::ExactMatch));
    modeComboBox->addItem(tr("Essence identity"), QVariant::fromValue(FilterMode::ColorIdentity));

    modeComboBox->setItemData(0, tr("Cards containing any of the selected Essences"), Qt::ToolTipRole);
    modeComboBox->setItemData(1, tr("Cards containing none of the selected Essences"), Qt::ToolTipRole);
    modeComboBox->setItemData(2, tr("Cards containing any of the selected Essences and none of the unselected Essences"),
                              Qt::ToolTipRole);
    modeComboBox->setItemData(3, tr("Cards having exactly the selected Essences"), Qt::ToolTipRole);
    modeComboBox->setItemData(4, tr("Cards whose Essence identity contains only the selected Essences"), Qt::ToolTipRole);

    if (currentMode == FilterMode::Custom) {
        addCustomFilterModeItem();
    }

    modeComboBox->setAccessibleName(tr("Essence filter mode"));

    // Restore current mode
    const int index = modeComboBox->findData(QVariant::fromValue(currentMode));
    if (index >= 0) {
        modeComboBox->setCurrentIndex(index);
    }
    modeComboBox->blockSignals(false);
}

void VisualDatabaseDisplayColorFilterWidget::setFilterMode(FilterMode filterMode)
{
    int index = modeComboBox->findData(QVariant::fromValue(filterMode));

    if (index < 0 && filterMode == FilterMode::Custom) {
        index = modeComboBox->count();
        addCustomFilterModeItem();
    }

    if (index >= 0) {
        modeComboBox->setCurrentIndex(index);
    }
}

void VisualDatabaseDisplayColorFilterWidget::addCustomFilterModeItem()
{
    int customIndex = modeComboBox->count();
    modeComboBox->addItem(tr("Custom"), QVariant::fromValue(FilterMode::Custom));
    modeComboBox->setItemData(customIndex, tr("Cards matching an unknown combination of Essence filters"),
                              Qt::ToolTipRole);
}

void VisualDatabaseDisplayColorFilterWidget::handleColorToggled(QChar essence, bool active)
{
    if (active) {
        addFilter(essence);
    } else {
        removeFilter(essence);
    }
}

void VisualDatabaseDisplayColorFilterWidget::addFilter(QChar essence)
{
    FilterModeInfo info = getFilterModeInfo(currentMode);

    removeFilters(filterModel, essence, info.attr);

    if (info.includeType != CardFilter::Type::TypeEnd) {
        filterModel->addFilter(new CardFilter(essence, info.includeType, info.attr));
    }
}

void VisualDatabaseDisplayColorFilterWidget::removeFilter(QChar essence)
{
    FilterModeInfo info = getFilterModeInfo(currentMode);

    removeFilters(filterModel, essence, info.attr);

    // An empty picker means any Essence, including in modes that otherwise exclude unselected types.
    if (essencePicker->activeEssences().isEmpty()) {
        filterModel->clearFiltersOfType(CardFilter::Attr::AttrColor);
        filterModel->clearFiltersOfType(CardFilter::Attr::AttrColorIdentity);
        return;
    }

    // Add exclusion filters if the mode demands it.
    if (info.excludeType != CardFilter::Type::TypeEnd) {
        filterModel->addFilter(new CardFilter(essence, info.excludeType, info.attr));
    }
}

void VisualDatabaseDisplayColorFilterWidget::updateFilterMode()
{
    const QVariant data = modeComboBox->currentData();
    if (!data.isValid()) {
        return;
    }

    currentMode = data.value<FilterMode>();

    filterModel->blockSignals(true);
    filterModel->filterTree()->blockSignals(true);

    filterModel->clearFiltersOfType(CardFilter::Attr::AttrColor);
    filterModel->clearFiltersOfType(CardFilter::Attr::AttrColorIdentity);

    const QSet<QChar> selected = essencePicker->activeEssences();
    // No selections should never generate 15 mutually excluding filters.
    if (!selected.isEmpty()) {
        for (const QChar essence : Essence::symbols()) {
            handleColorToggled(essence, selected.contains(essence));
        }
    }

    filterModel->blockSignals(false);
    filterModel->filterTree()->blockSignals(false);

    emit filterModel->filterTree()->changed();
    emit filterModel->layoutChanged();
    emit filterModeChanged(currentMode);
}

void VisualDatabaseDisplayColorFilterWidget::setEssenceActive(QChar essence, bool active)
{
    essencePicker->setEssenceActive(essence, active);
}

QList<QChar> VisualDatabaseDisplayColorFilterWidget::getActiveColors()
{
    QList<QChar> result;
    const QSet<QChar> selected = essencePicker->activeEssences();
    for (const QChar essence : Essence::symbols()) {
        if (selected.contains(essence)) {
            result.append(essence);
        }
    }
    return result;
}

void VisualDatabaseDisplayColorFilterWidget::syncWithFilterModel()
{
    QList<const CardFilter *> filters;
    filters.append(filterModel->getFiltersOfType(CardFilter::Attr::AttrColor));
    filters.append(filterModel->getFiltersOfType(CardFilter::Attr::AttrColorIdentity));

    // Re-derive the mode from the filters in the model, so that a loaded filter set selects the mode that
    // produced it (or Custom when none could have).
    currentMode = deriveFilterMode(filters, currentMode);

    // Update the combobox. Add a Custom item only if we have a custom filter mode.
    int customIndex = modeComboBox->findData(QVariant::fromValue(FilterMode::Custom));
    modeComboBox->blockSignals(true);
    if (currentMode == FilterMode::Custom) {
        if (customIndex < 0) {
            customIndex = modeComboBox->count();
            addCustomFilterModeItem();
        }
        modeComboBox->setCurrentIndex(customIndex);
    } else {
        if (customIndex >= 0) {
            modeComboBox->removeItem(customIndex);
        }
        int index = modeComboBox->findData(QVariant::fromValue(currentMode));
        if (index >= 0) {
            modeComboBox->setCurrentIndex(index);
        }
    }
    modeComboBox->blockSignals(false);

    // Restore the compact Essence picker without changing the filter model.
    const FilterModeInfo modeInfo = getFilterModeInfo(currentMode);

    const bool useExclude =
        modeInfo.includeType == CardFilter::Type::TypeEnd && modeInfo.excludeType != CardFilter::Type::TypeEnd;
    const CardFilter::Type filterType = useExclude ? modeInfo.excludeType : modeInfo.includeType;

    filters = filterModel->getFiltersOfType(modeInfo.attr);

    for (const QChar essence : Essence::symbols()) {
        bool found = false;
        for (const CardFilter *filter : filters) {
            if (filter->type() == filterType && essence == filter->term()) {
                found = true;
                break;
            }
        }

        setEssenceActive(essence, !filters.isEmpty() && (useExclude ? !found : found));
    }
    essencePicker->setEnabled(currentMode != FilterMode::Custom);
}
