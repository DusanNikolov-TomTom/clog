#include "yearly_view.hpp"

#include "calendar_component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/flexbox_config.hpp"
#include "ftxui_ext/extended_containers.hpp"
#include "highlight_menu.hpp"

#include <ftxui/screen/terminal.hpp>
#include <sstream>

namespace clog::view {
YearView::YearView(const model::Date& today) :
    m_screen { ScreenInteractive::Fullscreen() }, m_calendarButtons { Calendar::make(
                                                      today, makeCalendarOptions(today)) },
    m_tagsMenu { makeSectionsMenu() }, m_sectionsMenu { makeSectionsMenu() }, m_rootComponent {
        makeFullUIComponent()
    } {}

void YearView::run() { m_screen.Loop(m_rootComponent); }

void YearView::stop() { m_screen.ExitLoopClosure()(); }

void YearView::showCalendarForYear(unsigned year) { m_calendarButtons->displayYear(year); }

void YearView::setPreviewString(const std::string& string) {
    Elements lines;
    std::istringstream input { string };
    for (std::string line; std::getline(input, line);) {
        lines.push_back(text(line));
    }
    m_logFileContentsPreview = vbox(lines) | flex_shrink | border | size(HEIGHT, EQUAL, 10);
}

std::shared_ptr<Promptable> YearView::makeFullUIComponent() {
    auto container = ftxui_ext::CustomContainer(
        {
            m_tagsMenu,
            m_sectionsMenu,
            m_calendarButtons,
        },
        Event::Tab, Event::TabReverse);

    auto whole_ui_renderer = Renderer(container, [this, container] {
        std::stringstream date;
        date << "Today: " << model::Date::getToday().formatToString("%d. %m. %Y.");
        // preview window can sometimes be wider than the menus & calendar, it's simpler to keep
        // them centered while the preview window changes and stretches this vbox container than to
        // keep the preview window size fixed
        return vbox(text(date.str()) | center, container->Render() | center,
                    m_logFileContentsPreview) |
               center;
    });

    auto event_handler = CatchEvent(whole_ui_renderer, [&](Event e) {
        // TODO: why is e.input() a string? Will i miss some data if i just take the first char?
        // TODO: gotta prevent tab reverse error
        if (not e.is_mouse() && e != Event::TabReverse) {
            return m_handler->handleInputEvent({ UIEvent::ROOT_EVENT, e.input().front() });
        };
        return false;
    });

    return std::make_shared<Promptable>(event_handler);
}

CalendarOption YearView::makeCalendarOptions(const Date& today) {
    CalendarOption option;
    option.transform = [this, today](const auto& date, const auto& state) {
        auto element = text(state.label);
        if (state.focused)
            element = element | inverted;
        if (today == date)
            element = element | color(Color::Red);
        if (m_highlightedLogsMap && m_highlightedLogsMap->get(date))
            element = element | color(Color::Yellow);
        if (m_availabeLogsMap && !m_availabeLogsMap->get(date))
            element = element | dim;
        return element | center;
    };
    // TODO: Ignoring the provided new date only for the controller to ask
    // for new date is ugly
    option.focusChange = [this](const auto& /* date */) {
        m_handler->handleInputEvent({ UIEvent::FOCUSED_DATE_CHANGE });
    };
    option.enter = [this](const auto& /* date */) {
        m_handler->handleInputEvent({ UIEvent::CALENDAR_BUTTON_CLICK });
    };
    return std::move(option);
}

std::shared_ptr<WindowedMenu> YearView::makeTagsMenu() {
    MenuOption option { .on_change = [this] {
        m_handler->handleInputEvent({ UIEvent::FOCUSED_TAG_CHANGE, m_tagsMenu->selected() });
    } };
    return WindowedMenu::make("Tags", &m_tags, option);
}

std::shared_ptr<WindowedMenu> YearView::makeSectionsMenu() {
    MenuOption option = { .on_change = [this] {
        m_handler->handleInputEvent(
            { UIEvent::FOCUSED_SECTION_CHANGE, m_sectionsMenu->selected() });
    } };
    return WindowedMenu::make("Sections", &m_sections, option);
}
}  // namespace clog::view