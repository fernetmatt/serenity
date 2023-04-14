/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/URL.h>
#include <LibCore/ElapsedTimer.h>
#include <LibGUI/ImageWidget.h>
#include <LibGUI/Progressbar.h>
#include <LibGUI/Widget.h>
#include <LibWeb/Loader/ResourceLoader.h>

namespace Browser {

class DownloadWidget final : public GUI::Widget {
    C_OBJECT_ABSTRACT(DownloadWidget);

public:
    static ErrorOr<NonnullRefPtr<DownloadWidget>> try_create(const URL&);
    virtual ~DownloadWidget() override = default;

private:
    explicit DownloadWidget();

    void did_progress(Optional<u32> total_size, u32 downloaded_size);
    void did_finish(bool success);

    URL m_url;
    DeprecatedString m_destination_path;
    RefPtr<Web::ResourceLoaderConnectorRequest> m_download;
    RefPtr<GUI::Progressbar> m_progressbar;
    RefPtr<GUI::Label> m_progress_label;
    RefPtr<GUI::Button> m_cancel_button;
    RefPtr<GUI::Button> m_close_button;
    RefPtr<GUI::CheckBox> m_close_on_finish_checkbox;
    RefPtr<GUI::ImageWidget> m_browser_image;
    OwnPtr<Core::File> m_output_file_stream;
    Core::ElapsedTimer m_elapsed_timer;
};

}
