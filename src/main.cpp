#include <ddui/core>
#include <ddui/app>

void update() {

    // Writing hello world
    ddui::font_face("regular");
    ddui::font_size(48.0);
    ddui::fill_color(ddui::rgb(0x000000));
    ddui::text_align(ddui::align::CENTER | ddui::align::MIDDLE);
    ddui::text(ddui::view.width / 2, ddui::view.height / 2, "Hello, world!", NULL);

}

int main(int argc, const char** argv) {

    // ddui (graphics and UI system)
    if (!ddui::app_init(700, 300, "BMJ's Audio Programming", update)) {
        printf("Failed to init ddui.\n");
        return 0;
    }

    // Type faces
    ddui::create_font("regular", "SFRegular.ttf");
    ddui::create_font("medium", "SFMedium.ttf");
    ddui::create_font("bold", "SFBold.ttf");
    ddui::create_font("thin", "SFThin.ttf");
    ddui::create_font("mono", "PTMono.ttf");

    ddui::app_run();

    return 0;
}
