/*
    Copyright 2021 XITRIX

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <borealis/core/application.hpp>
#include <borealis/core/touch/hover_gesture.hpp>
#include <borealis/core/touch/tap_gesture.hpp>
#include <borealis/views/recycler.hpp>

namespace brls
{

RecyclerCell::RecyclerCell()
{
    if (Application::getStyle()["brls/recycler_cell/seperator"])
    {
        this->setLineBottom(1);
        this->setLineColor(Application::getTheme()["brls/sidebar/separator"]);
    }

    setHeight(Application::getStyle()["brls/dropdown/listItemHeight"]);

    this->registerClickAction(
        [this](View* view)
        {
            RecyclerFrame* recycler = dynamic_cast<RecyclerFrame*>(getParent()->getParent());
            if (recycler)
                recycler->getDataSource()->didSelectRowAt(recycler, m_indexPath);
            return true;
        }
    );

    m_subscription = Application::getGlobalInputTypeChangeEvent()->subscribe(
        [this](InputType type)
        {
            bool isTouch = type == InputType::TOUCH;
            this->setLineColor((!isTouch && this->focused) ? TRANSPARENT : Application::getTheme()["brls/sidebar/separator"]);
        }
    );

    this->addGestureRecognizer(new TapGestureRecognizer(this));
    this->addGestureRecognizer(new HoverGestureRecognizer(this));
}

RecyclerCell::~RecyclerCell() { Application::getGlobalInputTypeChangeEvent()->unsubscribe(m_subscription); }

RecyclerCell* RecyclerCell::create() { return new RecyclerCell(); }

void RecyclerCell::setIndexPath(IndexPath value)
{
    m_indexPath = value;

    this->setLineTop(value.row == 0 ? 1 : 0);
}

void RecyclerCell::onFocusGained()
{
    // Called when a child of ours gets focused, in that case it's the Image

    Box::onFocusGained();

    bool isTouch = Application::getInputType() == InputType::TOUCH;
    this->setLineColor(!isTouch ? TRANSPARENT : Application::getTheme()["brls/sidebar/separator"]);
}

void RecyclerCell::onFocusLost()
{
    // Called when a child of ours losts focused, in that case it's the Image

    Box::onFocusLost();
    this->setLineColor(Application::getTheme()["brls/sidebar/separator"]);
}

RecyclerHeader::RecyclerHeader()
{
    m_header = new Header();
    this->addView(m_header);
    m_header->setGrow(1);
}

void RecyclerHeader::setTitle(const std::string& title) { m_header->setTitle(title); }

void RecyclerHeader::setSubtitle(const std::string& subtitle) { m_header->setSubtitle(subtitle); }

RecyclerHeader* RecyclerHeader::create() { return new RecyclerHeader(); }

RecyclerCell* RecyclerDataSource::cellForHeader(RecyclerFrame* recycler, int section)
{
    RecyclerHeader* header = (RecyclerHeader*) recycler->dequeueReusableCell("brls::Header");
    std::string title      = this->titleForHeader(recycler, section);
    header->setTitle(title);
    header->setVisibility(title.empty() ? Visibility::GONE : Visibility::VISIBLE);
    header->setHeight(title.empty() ? 0 : View::AUTO);
    return header;
}

float RecyclerDataSource::heightForHeader(RecyclerFrame* recycler, int section)
{
    if (section == 0)
        return 0;
    return 44;
}

RecyclerContentBox::RecyclerContentBox(RecyclerFrame* recycler) : Box(Axis::COLUMN), m_recycler(recycler) {}

View* RecyclerContentBox::getNextFocus(FocusDirection direction, View* currentView)
{
    return m_recycler->getNextCellFocus(direction, currentView);
}

View* RecyclerFrame::getNextCellFocus(FocusDirection direction, View* currentView)
{
    void* parentUserData = currentView->getParentUserData();

    // Return nullptr immediately if focus direction mismatches the box axis (clang-format refuses to split it in multiple lines...)
    if ((m_contentBox->getAxis() == Axis::ROW && direction != FocusDirection::LEFT && direction != FocusDirection::RIGHT)
        || (m_contentBox->getAxis() == Axis::COLUMN && direction != FocusDirection::UP && direction != FocusDirection::DOWN))
    {
        View* next = getParentNavigationDecision(this, nullptr, direction);
        if (!next && hasParent())
            next = getParent()->getNextFocus(direction, this);
        return next;
    }

    // Traverse the children
    size_t offset = 1; // which way we are going in the children list

    if ((m_contentBox->getAxis() == Axis::ROW && direction == FocusDirection::LEFT)
        || (m_contentBox->getAxis() == Axis::COLUMN && direction == FocusDirection::UP))
    {
        offset = -1;
    }

    size_t currentFocusIndex = *((size_t*) parentUserData) + offset;
    View* currentFocus       = nullptr;

    while (!currentFocus && currentFocusIndex >= 0 && currentFocusIndex < m_cacheIndexPathData.size())
    {
        for (auto it : m_contentBox->getChildren())
        {
            if (*((size_t*) it->getParentUserData()) == currentFocusIndex)
            {
                currentFocus = it->getDefaultFocus();
                break;
            }
        }
        currentFocusIndex += offset;
    }

    currentFocus = getParentNavigationDecision(this, currentFocus, direction);
    if (!currentFocus && hasParent())
        currentFocus = getParent()->getNextFocus(direction, this);
    return currentFocus;
}

RecyclerFrame::RecyclerFrame()
{
    registerCell("brls::Header", []() { return RecyclerHeader::create(); });

    // Padding
    this->registerFloatXMLAttribute("paddingTop", [this](float value) { this->setPaddingTop(value); });

    this->registerFloatXMLAttribute("paddingRight", [this](float value) { this->setPaddingRight(value); });

    this->registerFloatXMLAttribute("paddingBottom", [this](float value) { this->setPaddingBottom(value); });

    this->registerFloatXMLAttribute("paddingLeft", [this](float value) { this->setPaddingLeft(value); });

    this->registerFloatXMLAttribute("padding", [this](float value) { this->setPadding(value); });

    this->setScrollingBehavior(ScrollingBehavior::CENTERED);

    // Create content box
    m_contentBox = new RecyclerContentBox(this);
    this->setContentView(m_contentBox);
}

RecyclerFrame::~RecyclerFrame()
{
    if (m_dataSource && m_deleteDataSource)
        delete m_dataSource;

    for (auto it : m_queueMap)
    {
        for (auto item : *it.second)
            delete item;
        delete it.second;
    }
}

void RecyclerFrame::setDataSource(RecyclerDataSource* source, bool deleteDataSource)
{
    if (m_dataSource && m_deleteDataSource)
        delete m_dataSource;

    m_dataSource       = source;
    m_deleteDataSource = deleteDataSource;
    if (m_layouted)
        reloadData();
}

RecyclerDataSource* RecyclerFrame::getDataSource() const { return m_dataSource; }

void RecyclerFrame::reloadData()
{
    if (!m_layouted)
        return;

    auto children = m_contentBox->getChildren();
    for (auto const& child : children)
    {
        queueReusableCell((RecyclerCell*) child);
        m_contentBox->removeView(child, false);
    }

    m_visibleMin = std::numeric_limits<size_t>::max();
    m_visibleMax = 0;

    m_renderedFrame            = Rect();
    m_renderedFrame.size.width = getWidth();

    setContentOffsetY(0, false);

    if (m_dataSource)
    {
        cacheCellFrames();
        Rect frame  = getLocalFrame();
        int counter = 0;
        for (int section = 0; section < m_dataSource->numberOfSections(this); section++)
        {
            for (int row = -1; row < m_dataSource->numberOfRows(this, section); row++)
            {
                addCellAt(counter++, true);
                if (m_renderedFrame.getMaxY() > frame.getMaxY())
                    break;
            }
        }

        selectRowAt(m_defaultCellFocus, false);
    }
}

void RecyclerFrame::registerCell(const std::string& identifier, const std::function<RecyclerCell*()>& allocation)
{
    m_queueMap.insert(std::make_pair(identifier, new std::vector<RecyclerCell*>()));
    m_allocationMap.insert(std::make_pair(identifier, allocation));
}

RecyclerCell* RecyclerFrame::dequeueReusableCell(const std::string& identifier)
{
    RecyclerCell* cell = nullptr;
    auto it            = m_queueMap.find(identifier);

    if (it != m_queueMap.end())
    {
        std::vector<RecyclerCell*>* vector = it->second;
        if (!vector->empty())
        {
            cell = vector->back();
            vector->pop_back();
        }
        else
        {
            cell                  = m_allocationMap.at(identifier)();
            cell->reuseIdentifier = identifier;
            cell->detach();
        }
    }

    if (cell)
        cell->prepareForReuse();

    return cell;
}

// TODO: Implement it normally
void RecyclerFrame::selectRowAt(IndexPath indexPath, bool animated)
{
    size_t count = 0;
    float offset = 0;

    for (size_t j = 0; j < indexPath.section; j++)
        for (int i = -1; i < (m_dataSource->numberOfRows(this, j)); i++)
        {
            offset += m_cacheFramesData[count++].height;
        }

    for (int i = -1; i <= indexPath.row; i++)
        offset += m_cacheFramesData[count++].height;

    offset -= this->getHeight() / 2;
    this->setContentOffsetY(offset, animated);
    this->cellsRecyclingLoop();

    for (View* view : m_contentBox->getChildren())
    {
        if (*((size_t*) view->getParentUserData()) == count - 1)
        {
            m_contentBox->setLastFocusedView(view);
            break;
        }
    }
}

void RecyclerFrame::queueReusableCell(RecyclerCell* cell) { m_queueMap.at(cell->reuseIdentifier)->push_back(cell); }

void RecyclerFrame::cacheCellFrames()
{
    m_cacheFramesData.clear();
    m_cacheIndexPathData.clear();
    Rect frame = getFrame();
    Point currentOrigin;

    if (m_dataSource)
    {
        for (int section = 0; section < m_dataSource->numberOfSections(this); section++)
        {
            for (int row = -1; row < m_dataSource->numberOfRows(this, section); row++)
            {
                m_cacheIndexPathData.push_back(IndexPath(section, row));

                float height
                    = row == -1 ? m_dataSource->heightForHeader(this, section) : m_dataSource->heightForRow(this, IndexPath(section, row));

                if (height == -1)
                    height = estimatedRowHeight;

                m_cacheFramesData.push_back(Size(frame.getWidth(), height));
                currentOrigin.y += height;
            }
        }
        m_contentBox->setHeight(currentOrigin.y + m_paddingTop + m_paddingBottom);
    }
}

bool RecyclerFrame::checkWidth()
{
    float width           = getWidth();
    static float oldWidth = width;
    if ((int) oldWidth != (int) width && width != 0)
    {
        oldWidth = width;
        return true;
    }
    oldWidth = width;
    return false;
}

void RecyclerFrame::cellsRecyclingLoop()
{
    Rect visibleFrame = getVisibleFrame();

    while (true)
    {
        RecyclerCell* minCell = nullptr;
        for (auto it : m_contentBox->getChildren())
            if (*((size_t*) it->getParentUserData()) == m_visibleMin)
                minCell = (RecyclerCell*) it;

        if (!minCell || minCell->getDetachedPosition().y + minCell->getHeight() >= visibleFrame.getMinY())
            break;

        float cellHeight = minCell->getHeight();
        m_renderedFrame.origin.y += cellHeight;
        m_renderedFrame.size.height -= cellHeight;

        queueReusableCell(minCell);
        m_contentBox->removeView(minCell, false);

        Logger::debug("Cell #{} - destroyed", m_visibleMin);

        m_visibleMin++;
    }

    while (true)
    {
        RecyclerCell* maxCell = nullptr;
        for (auto it : m_contentBox->getChildren())
            if (*((size_t*) it->getParentUserData()) == m_visibleMax)
                maxCell = (RecyclerCell*) it;

        if (!maxCell || maxCell->getDetachedPosition().y <= visibleFrame.getMaxY())
            break;

        float cellHeight = maxCell->getHeight();
        m_renderedFrame.size.height -= cellHeight;

        queueReusableCell(maxCell);
        m_contentBox->removeView(maxCell, false);

        Logger::debug("Cell #{} - destroyed", m_visibleMax);

        m_visibleMax--;
    }

    while (m_visibleMin - 1 < m_cacheFramesData.size() && m_renderedFrame.getMinY() > visibleFrame.getMinY() - m_paddingTop)
    {
        size_t i = m_visibleMin - 1;
        addCellAt(i, false);
    }

    while (m_visibleMax + 1 < m_cacheFramesData.size() && m_renderedFrame.getMaxY() < visibleFrame.getMaxY() - m_paddingBottom)
    {
        size_t i = m_visibleMax + 1;
        addCellAt(i, true);
    }
}

void RecyclerFrame::addCellAt(size_t index, size_t downSide)
{
    IndexPath indexPath = m_cacheIndexPathData[index];

    RecyclerCell* cell;
    if (indexPath.row == -1)
        cell = m_dataSource->cellForHeader(this, indexPath.section);
    else
    {
        cell = m_dataSource->cellForRow(this, indexPath);
        cell->setLineBottom(1);
    }

    cell->setWidth(m_renderedFrame.getWidth() - m_paddingLeft - m_paddingRight);
    Point cellOrigin = Point(
        m_renderedFrame.getMinX() + m_paddingLeft,
        (downSide ? m_renderedFrame.getMaxY() : m_renderedFrame.getMinY() - cell->getHeight()) + m_paddingTop
    );

    cell->setDetachedPosition(cellOrigin.x, cellOrigin.y);
    cell->setIndexPath(indexPath);

    m_contentBox->getChildren().insert(m_contentBox->getChildren().end(), cell);

    // Allocate and set parent userdata
    size_t* userdata = (size_t*) malloc(sizeof(size_t));
    *userdata        = index;

    cell->setParent(m_contentBox, userdata);

    // Layout and events
    m_contentBox->invalidate();
    cell->View::willAppear();

    if (index < m_visibleMin)
        m_visibleMin = index;

    if (index > m_visibleMax)
        m_visibleMax = index;

    Rect cellFrame = cell->getFrame();

    if (!downSide)
        m_renderedFrame.origin.y -= cellFrame.getHeight();

    m_renderedFrame.size.height += cellFrame.getHeight();

    if (cellFrame.getHeight() != m_cacheFramesData[index].height)
    {
        float delta = cellFrame.getHeight() - m_cacheFramesData[index].height;
        m_contentBox->setHeight(m_contentBox->getHeight() + delta);
        m_cacheFramesData[index].height = cellFrame.getHeight();
    }

    Logger::debug("Cell #{} - added", index);
}

void RecyclerFrame::onLayout()
{
    ScrollingFrame::onLayout();
    m_contentBox->setWidth(this->getWidth());
    if (checkWidth())
    {
        m_layouted = true;
        reloadData();
    }
}

void RecyclerFrame::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    cellsRecyclingLoop();
    ScrollingFrame::draw(vg, x, y, width, height, style, ctx);
}

void RecyclerFrame::setPadding(float padding) { this->setPadding(padding, padding, padding, padding); }

void RecyclerFrame::setPadding(float top, float right, float bottom, float left)
{
    m_paddingTop    = top;
    m_paddingRight  = right;
    m_paddingBottom = bottom;
    m_paddingLeft   = left;

    this->reloadData();
}

void RecyclerFrame::setPaddingTop(float top)
{
    m_paddingTop = top;
    this->reloadData();
}

void RecyclerFrame::setPaddingRight(float right)
{
    m_paddingRight = right;
    this->reloadData();
}

void RecyclerFrame::setPaddingBottom(float bottom)
{
    m_paddingBottom = bottom;
    this->reloadData();
}

void RecyclerFrame::setPaddingLeft(float left)
{
    m_paddingLeft = left;
    this->reloadData();
}

View* RecyclerFrame::create() { return new RecyclerFrame(); }

} // namespace brls
