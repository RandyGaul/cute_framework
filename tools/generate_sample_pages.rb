#!/usr/bin/env ruby
# frozen_string_literal: true

# Usage: ./generate_sample_pages.rb <build_dir> <docs_dir> [repo_url] [mkdocs_yml]
#
# Reads the sample list from the Samples: section of mkdocs.yml nav, which is
# the source of truth for which samples to generate pages for.

require 'cgi'
require 'fileutils'
require 'yaml'

# Target name mappings for cases where the nav target differs from the file name
# e.g. nine_slice -> 9_slice
TARGET_MAP = { 'nine_slice' => '9_slice' }.freeze

# Extract only the nav: block from mkdocs.yml so we avoid Python-specific YAML
# tags (!!python/name:, !!python/object/apply:) present in other sections.
def extract_nav_section(mkdocs_path)
  in_nav = false
  nav_lines = []

  File.foreach(mkdocs_path) do |line|
    if line.start_with?('nav:')
      in_nav = true
      nav_lines << line
    elsif in_nav
      break if line.match?(/^\S/) # next top-level key ends the nav block
      nav_lines << line
    end
  end

  YAML.safe_load(nav_lines.join)
end

def parse_samples_from_nav(nav)
  samples_section = (nav['nav'] || []).find { |e| e.is_a?(Hash) && e.key?('Samples') }
  return [] unless samples_section

  samples = []
  (samples_section['Samples'] || []).each do |entry|
    next if entry.is_a?(String) # skip bare paths like samples/index.md
    entry.each do |display_name, path|
      target = File.basename(path.to_s, '.md')
      next if target == 'index'
      samples << { target: target, name: display_name }
    end
  end
  samples
end

def find_source_file(target, samples_src_dir)
  actual_target = TARGET_MAP.fetch(target, target)
  %w[c cpp].each do |ext|
    file = "#{actual_target}.#{ext}"
    return file if File.exist?(File.join(samples_src_dir, file))
  end
  nil
end

def main
  if ARGV.length < 2
    warn "Usage: #{$PROGRAM_NAME} <build_dir> <docs_dir> [repo_url] [mkdocs_yml]"
    exit 1
  end

  build_dir = ARGV[0]
  docs_dir = ARGV[1]
  repo_url = ARGV[2] || 'https://github.com/RandyGaul/cute_framework'
  mkdocs_yml = ARGV[3] || File.expand_path('../mkdocs.yml', __dir__)

  samples_src_dir = File.expand_path('../samples', __dir__)
  samples_dir = File.join(docs_dir, 'samples')
  play_dir = File.join(samples_dir, 'play')

  FileUtils.rm_rf(play_dir)
  FileUtils.mkdir_p(play_dir)

  emscripten_dir = File.join(build_dir, 'emscripten')
  FileUtils.cp_r(emscripten_dir, play_dir) if Dir.exist?(emscripten_dir)

  nav = extract_nav_section(mkdocs_yml)
  samples = parse_samples_from_nav(nav)

  if samples.empty?
    warn "Warning: No samples found in Samples: nav section of #{mkdocs_yml}"
    exit 0
  end

  generated_samples = []

  samples.each do |sample|
    target = sample[:target]
    display_name = sample[:name]
    html_name = CGI.escapeHTML(display_name)
    actual_target = TARGET_MAP.fetch(target, target)

    source_file = find_source_file(target, samples_src_dir)
    if source_file.nil?
      warn "Warning: No source file found for #{target} in #{samples_src_dir}, skipping"
      next
    end

    html_file = File.join(build_dir, "#{actual_target}.html")
    unless File.exist?(html_file)
      warn "Warning: #{html_file} not found, skipping #{target}"
      next
    end

    %w[html js wasm data].each do |ext|
      artifact = File.join(build_dir, "#{actual_target}.#{ext}")
      FileUtils.cp(artifact, play_dir) if File.exist?(artifact)
    end

    markdown_content = <<~MARKDOWN
      ---
      hide:
        - toc
      ---

      # #{html_name}

      <div class="sample-container">
        <iframe class="sample-iframe" src="../play/#{actual_target}.html" title="#{html_name} Sample"></iframe>
      </div>

      <div class="sample-controls" markdown>
        [:material-fullscreen: Fullscreen](../play/#{actual_target}.html){: .md-button target="_blank" }
        [:material-code-tags: View Source](#{repo_url}/blob/master/samples/#{source_file}){: .md-button target="_blank" }
      </div>
    MARKDOWN

    File.write(File.join(samples_dir, "#{target}.md"), markdown_content)
    puts "Generated #{target}.md"
    generated_samples << { target: target, name: display_name }
  end

  sorted_samples = generated_samples.sort_by { |s| s[:name] }

  index_header = <<~MARKDOWN
    # Samples

    Interactive samples demonstrating Cute Framework features. Click any card to play the sample in your browser.

    <div class="grid cards" markdown>
  MARKDOWN

  cards = sorted_samples.map do |sample|
    <<~CARD

      - **[#{CGI.escapeHTML(sample[:name])}](#{sample[:target]}.md)**

          ---

          Interactive sample demonstrating #{CGI.escapeHTML(sample[:name].downcase)} features.

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

  File.write(File.join(samples_dir, 'index.md'), index_header + cards + index_footer)
  puts "Generated index.md with #{sorted_samples.length} samples"
end

main if __FILE__ == $PROGRAM_NAME
