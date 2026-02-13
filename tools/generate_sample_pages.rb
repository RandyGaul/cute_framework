#!/usr/bin/env ruby
# frozen_string_literal: true

# Usage: ./generate_sample_pages.rb <build_dir> <docs_dir> [repo_url]

require 'fileutils'

# Sample metadata
SAMPLES = [
  { target: 'easysprite', name: 'Easy Sprite', source: 'easy_sprite.c' },
  { target: 'basicserialization', name: 'Basic Serialization', source: 'basic_serialization.c' },
  { target: 'imgui', name: 'Dear ImGui', source: 'imgui.c' },
  { target: 'textdrawing', name: 'Text Drawing', source: 'text_drawing.cpp' },
  { target: 'basicsprite', name: 'Basic Sprite', source: 'basic_sprite.cpp' },
  { target: 'basicshapes', name: 'Basic Shapes', source: 'basic_shapes.cpp' },
  { target: 'windowresizing', name: 'Window Resizing', source: 'window_resizing.cpp' },
  { target: 'basicinput', name: 'Basic Input', source: 'basic_input.c' },
  { target: 'windowevents', name: 'Window Events', source: 'window_events.c' },
  { target: 'window', name: 'Window', source: 'window.cpp' },
  { target: 'scratch', name: 'Scratch', source: 'scratch.cpp' },
  { target: 'spaceshooter', name: 'Space Shooter', source: 'spaceshooter.cpp' },
  { target: 'draw_to_texture', name: 'Draw to Texture', source: 'draw_to_texture.c' },
  { target: 'hello_triangle', name: 'Hello Triangle', source: 'hello_triangle.c' },
  { target: 'basicinstancing', name: 'Basic Instancing', source: 'basic_instancing.c' },
  { target: 'basicindexedrendering', name: 'Basic Indexed Rendering', source: 'basic_indexed_rendering.c' },
  { target: 'waves', name: 'Waves', source: 'waves.cpp' },
  { target: 'shallow_water', name: 'Shallow Water', source: 'shallow_water.cpp' },
  { target: 'noise', name: 'Noise', source: 'noise.c' },
  { target: 'fetch_image', name: 'Fetch Image', source: 'fetch_image.cpp' },
  { target: 'metaballs', name: 'Metaballs', source: 'metaballs.cpp' },
  { target: 'timestep', name: 'Timestep', source: 'timestep.cpp' },
  { target: 'joypad', name: 'Joypad', source: 'joypad.c' },
  { target: 'outline_stencil', name: 'Outline Stencil', source: 'outline_stencil.cpp' },
  { target: 'recolor', name: 'Recolor', source: 'recolor.cpp' },
  { target: 'rainbow_liquid', name: 'Rainbow Liquid', source: 'rainbow_liquid.cpp' },
  { target: 'import_spritesheet', name: 'Import Spritesheet', source: 'import_spritesheet.cpp' },
  { target: 'stencil_pie_chart', name: 'Stencil Pie Chart', source: 'stencil_pie_chart.c' },
  { target: 'platformer', name: 'Platformer', source: 'platformer.cpp' },
  { target: 'polygon', name: 'Polygon', source: 'polygon.cpp' },
  { target: 'scissor', name: 'Scissor', source: 'scissor.c' },
  { target: 'ime', name: 'IME', source: 'ime.c' },
  { target: 'pivot', name: 'Pivot', source: 'pivot.cpp' },
  { target: 'fluid_sim', name: 'Fluid Simulation', source: 'fluid_sim.cpp' },
  { target: 'sprite_slice', name: 'Sprite Slice', source: 'sprite_slice.cpp' },
  { target: 'sprite_shatter', name: 'Sprite Shatter', source: 'sprite_shatter.cpp' },
  { target: 'screen_shatter', name: 'Screen Shatter', source: 'screen_shatter.cpp' },
  { target: 'font_debug', name: 'Font Debug', source: 'font_debug.cpp' },
  { target: 'clay', name: 'Clay UI', source: 'clay.c' },
  { target: 'clay_ui_animations', name: 'Clay UI Animations', source: 'clay_ui_animations.c' },
  { target: 'nine_slice', name: '9-Slice', source: '9_slice.c' },
  { target: 'basic_camera', name: 'Basic Camera', source: 'basic_camera.c' },
  { target: 'imgui_texture', name: 'ImGui Texture', source: 'imgui_texture.c' },
  { target: 'input_binding', name: 'Input Binding', source: 'input_binding.c' }
].freeze

# Target name mappings (e.g., nine_slice -> 9_slice for file lookup)
TARGET_MAP = { 'nine_slice' => '9_slice' }.freeze

# Exclude non-visual/broken samples
EXCLUDE_LIST = %w[basicserialization].freeze

def main
  if ARGV.length < 2
    warn "Usage: #{$PROGRAM_NAME} <build_dir> <docs_dir> [repo_url]"
    exit 1
  end

  build_dir = ARGV[0]
  docs_dir = ARGV[1]
  repo_url = ARGV[2] || 'https://github.com/RandyGaul/cute_framework'

  samples_dir = File.join(docs_dir, 'samples')
  play_dir = File.join(samples_dir, 'play')

  # Create output directories
  FileUtils.mkdir_p(play_dir)

  # Copy emscripten shell assets
  emscripten_dir = File.join(build_dir, 'emscripten')
  FileUtils.cp_r(emscripten_dir, play_dir) if Dir.exist?(emscripten_dir)

  # Process each sample
  generated_samples = []

  SAMPLES.each do |sample|
    target = sample[:target]
    display_name = sample[:name]
    source_file = sample[:source]

    # Skip excluded samples
    next if EXCLUDE_LIST.include?(target)

    # Get actual target name for file lookup
    actual_target = TARGET_MAP.fetch(target, target)

    # Check if built artifacts exist
    html_file = File.join(build_dir, "#{actual_target}.html")
    unless File.exist?(html_file)
      warn "Warning: #{html_file} not found, skipping #{target}"
      next
    end

    # Copy artifacts to play directory
    %w[html js wasm data].each do |ext|
      artifact = File.join(build_dir, "#{actual_target}.#{ext}")
      FileUtils.cp(artifact, play_dir) if File.exist?(artifact)
    end

    # Generate per-sample markdown page
    markdown_content = <<~MARKDOWN
      ---
      hide:
        - toc
      ---

      # #{display_name}

      <div class="sample-container">
        <iframe class="sample-iframe" src="../play/#{actual_target}.html" title="#{display_name} Sample"></iframe>
      </div>

      <div class="sample-controls" markdown>
        [:material-fullscreen: Fullscreen](../play/#{actual_target}.html){: .md-button target="_blank" }
        [:material-code-tags: View Source](#{repo_url}/blob/master/samples/#{source_file}){: .md-button target="_blank" }
      </div>
    MARKDOWN

    sample_md_path = File.join(samples_dir, "#{target}.md")
    File.write(sample_md_path, markdown_content)
    puts "Generated #{target}.md"

    generated_samples << { target: target, name: display_name }
  end

  # Generate index page with sorted cards
  sorted_samples = generated_samples.sort_by { |s| s[:name] }

  index_header = <<~MARKDOWN
    # Samples

    Interactive samples demonstrating Cute Framework features. Click any card to play the sample in your browser.

    <div class="grid cards" markdown>
  MARKDOWN

  cards = sorted_samples.map do |sample|
    display_name_lower = sample[:name].downcase

    <<~CARD

      - **[#{sample[:name]}](#{sample[:target]}.md)**

          ---

          Interactive sample demonstrating #{display_name_lower} features.

          [:octicons-arrow-right-24: Play](#{sample[:target]}.md)

    CARD
  end.join

  index_footer = <<~MARKDOWN
    </div>

    !!! note "Source Code"
        All sample source code is available in the [samples directory](https://github.com/RandyGaul/cute_framework/tree/master/samples) on GitHub.

    !!! tip "Build Locally"
        To build and run samples locally, see the [Getting Started](../getting_started.md) guide and [Web Builds with Emscripten](../topics/emscripten.md) topic.
  MARKDOWN

  index_content = index_header + cards + index_footer
  File.write(File.join(samples_dir, 'index.md'), index_content)
  puts "Generated index.md with #{sorted_samples.length} samples"

  # Update mkdocs.yml navigation with all sample pages
  system('ruby', 'tools/update_samples_nav.rb', docs_dir)
end

main if __FILE__ == $PROGRAM_NAME
