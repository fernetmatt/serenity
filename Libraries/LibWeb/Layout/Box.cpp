/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <LibGUI/Painter.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/HTML/HTMLBodyElement.h>
#include <LibWeb/Layout/BlockBox.h>
#include <LibWeb/Layout/Box.h>
#include <LibWeb/Page/Frame.h>

namespace Web::Layout {

void Box::paint_border(PaintContext& context, Edge edge, const Gfx::FloatRect& rect, const BorderData& border_data)
{
    float width = border_data.width;
    if (width <= 0)
        return;

    auto color = border_data.color;
    auto border_style = border_data.line_style;
    int int_width = max((int)width, 1);

    struct Points {
        Gfx::FloatPoint p1;
        Gfx::FloatPoint p2;
    };

    auto points_for_edge = [](Edge edge, const Gfx::FloatRect& rect) -> Points {
        switch (edge) {
        case Edge::Top:
            return { rect.top_left(), rect.top_right() };
        case Edge::Right:
            return { rect.top_right(), rect.bottom_right() };
        case Edge::Bottom:
            return { rect.bottom_left(), rect.bottom_right() };
        default: // Edge::Left
            return { rect.top_left(), rect.bottom_left() };
        }
    };

    auto [p1, p2] = points_for_edge(edge, rect);

    if (border_style == CSS::LineStyle::Inset) {
        auto top_left_color = Color::from_rgb(0x5a5a5a);
        auto bottom_right_color = Color::from_rgb(0x888888);
        color = (edge == Edge::Left || edge == Edge::Top) ? top_left_color : bottom_right_color;
    } else if (border_style == CSS::LineStyle::Outset) {
        auto top_left_color = Color::from_rgb(0x888888);
        auto bottom_right_color = Color::from_rgb(0x5a5a5a);
        color = (edge == Edge::Left || edge == Edge::Top) ? top_left_color : bottom_right_color;
    }

    auto gfx_line_style = Gfx::Painter::LineStyle::Solid;
    if (border_style == CSS::LineStyle::Dotted)
        gfx_line_style = Gfx::Painter::LineStyle::Dotted;
    if (border_style == CSS::LineStyle::Dashed)
        gfx_line_style = Gfx::Painter::LineStyle::Dashed;

    if (gfx_line_style != Gfx::Painter::LineStyle::Solid) {
        switch (edge) {
        case Edge::Top:
            p1.move_by(int_width / 2, int_width / 2);
            p2.move_by(-int_width / 2, int_width / 2);
            break;
        case Edge::Right:
            p1.move_by(-int_width / 2, int_width / 2);
            p2.move_by(-int_width / 2, -int_width / 2);
            break;
        case Edge::Bottom:
            p1.move_by(int_width / 2, -int_width / 2);
            p2.move_by(-int_width / 2, -int_width / 2);
            break;
        case Edge::Left:
            p1.move_by(int_width / 2, int_width / 2);
            p2.move_by(int_width / 2, -int_width / 2);
            break;
        }
        context.painter().draw_line({ (int)p1.x(), (int)p1.y() }, { (int)p2.x(), (int)p2.y() }, color, int_width, gfx_line_style);
        return;
    }

    auto draw_line = [&](auto& p1, auto& p2) {
        context.painter().draw_line({ (int)p1.x(), (int)p1.y() }, { (int)p2.x(), (int)p2.y() }, color, 1, gfx_line_style);
    };

    float p1_step = 0;
    float p2_step = 0;

    switch (edge) {
    case Edge::Top:
        p1_step = style().border_left().width / (float)int_width;
        p2_step = style().border_right().width / (float)int_width;
        for (int i = 0; i < int_width; ++i) {
            draw_line(p1, p2);
            p1.move_by(p1_step, 1);
            p2.move_by(-p2_step, 1);
        }
        break;
    case Edge::Right:
        p1_step = style().border_top().width / (float)int_width;
        p2_step = style().border_bottom().width / (float)int_width;
        for (int i = int_width - 1; i >= 0; --i) {
            draw_line(p1, p2);
            p1.move_by(-1, p1_step);
            p2.move_by(-1, -p2_step);
        }
        break;
    case Edge::Bottom:
        p1_step = style().border_left().width / (float)int_width;
        p2_step = style().border_right().width / (float)int_width;
        for (int i = int_width - 1; i >= 0; --i) {
            draw_line(p1, p2);
            p1.move_by(p1_step, -1);
            p2.move_by(-p2_step, -1);
        }
        break;
    case Edge::Left:
        p1_step = style().border_top().width / (float)int_width;
        p2_step = style().border_bottom().width / (float)int_width;
        for (int i = 0; i < int_width; ++i) {
            draw_line(p1, p2);
            p1.move_by(1, p1_step);
            p2.move_by(1, -p2_step);
        }
        break;
    }
}

void Box::paint(PaintContext& context, PaintPhase phase)
{
    if (!is_visible())
        return;

    Gfx::PainterStateSaver saver(context.painter());
    if (is_fixed_position())
        context.painter().translate(context.scroll_offset());

    Gfx::FloatRect padded_rect;
    padded_rect.set_x(absolute_x() - box_model().padding.left.to_px(*this));
    padded_rect.set_width(width() + box_model().padding.left.to_px(*this) + box_model().padding.right.to_px(*this));
    padded_rect.set_y(absolute_y() - box_model().padding.top.to_px(*this));
    padded_rect.set_height(height() + box_model().padding.top.to_px(*this) + box_model().padding.bottom.to_px(*this));

    if (phase == PaintPhase::Background && !is_body()) {
        // FIXME: We should paint the body here too, but that currently happens at the view layer.
        auto bgcolor = specified_style().property(CSS::PropertyID::BackgroundColor);
        if (bgcolor.has_value() && bgcolor.value()->is_color()) {
            context.painter().fill_rect(enclosing_int_rect(padded_rect), bgcolor.value()->to_color(document()));
        }

        auto bgimage = specified_style().property(CSS::PropertyID::BackgroundImage);
        if (bgimage.has_value() && bgimage.value()->is_image()) {
            auto& image_value = static_cast<const CSS::ImageStyleValue&>(*bgimage.value());
            if (image_value.bitmap()) {
                context.painter().draw_tiled_bitmap(enclosing_int_rect(padded_rect), *image_value.bitmap());
            }
        }
    }

    if (phase == PaintPhase::Border) {
        Gfx::FloatRect bordered_rect;
        bordered_rect.set_x(padded_rect.x() - box_model().border.left.to_px(*this));
        bordered_rect.set_width(padded_rect.width() + box_model().border.left.to_px(*this) + box_model().border.right.to_px(*this));
        bordered_rect.set_y(padded_rect.y() - box_model().border.top.to_px(*this));
        bordered_rect.set_height(padded_rect.height() + box_model().border.top.to_px(*this) + box_model().border.bottom.to_px(*this));

        paint_border(context, Edge::Left, bordered_rect, style().border_left());
        paint_border(context, Edge::Right, bordered_rect, style().border_right());
        paint_border(context, Edge::Top, bordered_rect, style().border_top());
        paint_border(context, Edge::Bottom, bordered_rect, style().border_bottom());
    }

    Layout::NodeWithStyleAndBoxModelMetrics::paint(context, phase);

    if (phase == PaintPhase::Overlay && dom_node() && document().inspected_node() == dom_node()) {
        auto content_rect = absolute_rect();

        auto margin_box = box_model().margin_box(*this);
        Gfx::FloatRect margin_rect;
        margin_rect.set_x(absolute_x() - margin_box.left);
        margin_rect.set_width(width() + margin_box.left + margin_box.right);
        margin_rect.set_y(absolute_y() - margin_box.top);
        margin_rect.set_height(height() + margin_box.top + margin_box.bottom);

        context.painter().draw_rect(enclosing_int_rect(margin_rect), Color::Yellow);
        context.painter().draw_rect(enclosing_int_rect(padded_rect), Color::Cyan);
        context.painter().draw_rect(enclosing_int_rect(content_rect), Color::Magenta);
    }

    if (phase == PaintPhase::FocusOutline && dom_node() && dom_node()->is_element() && downcast<DOM::Element>(*dom_node()).is_focused()) {
        context.painter().draw_rect(enclosing_int_rect(absolute_rect()), context.palette().focus_outline());
    }
}

HitTestResult Box::hit_test(const Gfx::IntPoint& position, HitTestType type) const
{
    // FIXME: It would be nice if we could confidently skip over hit testing
    //        parts of the layout tree, but currently we can't just check
    //        m_rect.contains() since inline text rects can't be trusted..
    HitTestResult result { absolute_rect().contains(position.x(), position.y()) ? this : nullptr };
    for_each_child([&](auto& child) {
        if (is<Box>(child) && downcast<Box>(child).stacking_context())
            return;
        auto child_result = child.hit_test(position, type);
        if (child_result.layout_node)
            result = child_result;
    });
    return result;
}

void Box::set_needs_display()
{
    if (!is_inline()) {
        frame().set_needs_display(enclosing_int_rect(absolute_rect()));
        return;
    }

    Node::set_needs_display();
}

bool Box::is_body() const
{
    return dom_node() && dom_node() == document().body();
}

void Box::set_offset(const Gfx::FloatPoint& offset)
{
    if (m_offset == offset)
        return;
    m_offset = offset;
    did_set_rect();
}

void Box::set_size(const Gfx::FloatSize& size)
{
    if (m_size == size)
        return;
    m_size = size;
    did_set_rect();
}

Gfx::FloatPoint Box::effective_offset() const
{
    if (m_containing_line_box_fragment)
        return m_containing_line_box_fragment->offset();
    return m_offset;
}

const Gfx::FloatRect Box::absolute_rect() const
{
    Gfx::FloatRect rect { effective_offset(), size() };
    for (auto* block = containing_block(); block; block = block->containing_block()) {
        rect.move_by(block->effective_offset());
    }
    return rect;
}

void Box::set_containing_line_box_fragment(LineBoxFragment& fragment)
{
    m_containing_line_box_fragment = fragment.make_weak_ptr();
}

StackingContext* Box::enclosing_stacking_context()
{
    for (auto* ancestor = parent(); ancestor; ancestor = ancestor->parent()) {
        if (!ancestor->is_box())
            continue;
        auto& ancestor_box = downcast<Box>(*ancestor);
        if (!ancestor_box.establishes_stacking_context())
            continue;
        ASSERT(ancestor_box.stacking_context());
        return ancestor_box.stacking_context();
    }
    // We should always reach the Layout::InitialContainingBlockBox stacking context.
    ASSERT_NOT_REACHED();
}

bool Box::establishes_stacking_context() const
{
    if (!has_style())
        return false;
    if (dom_node() == document().root())
        return true;
    auto position = style().position();
    auto z_index = style().z_index();
    if (position == CSS::Position::Absolute || position == CSS::Position::Relative) {
        if (z_index.has_value())
            return true;
    }
    if (position == CSS::Position::Fixed || position == CSS::Position::Sticky)
        return true;
    return false;
}

LineBox& Box::ensure_last_line_box()
{
    if (m_line_boxes.is_empty())
        return add_line_box();
    return m_line_boxes.last();
}

LineBox& Box::add_line_box()
{
    m_line_boxes.append(LineBox());
    return m_line_boxes.last();
}

float Box::width_of_logical_containing_block() const
{
    auto* containing_block = this->containing_block();
    ASSERT(containing_block);
    return containing_block->width();
}

}
