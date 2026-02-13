#!/usr/bin/env ruby
# frozen_string_literal: true

# Updates the mkdocs.yml navigation to include all generated sample pages.
# Usage: ./update_samples_nav.rb <docs_dir>

def main
  if ARGV.empty?
    warn "Usage: #{$PROGRAM_NAME} <docs_dir>"
    exit 1
  end

  docs_dir = ARGV[0]
  samples_dir = File.join(docs_dir, 'samples')
  mkdocs_yml = 'mkdocs.yml'

  unless File.exist?(mkdocs_yml)
    warn "Error: #{mkdocs_yml} not found"
    exit 1
  end

  # Read all sample markdown files (except index.md) and extract titles
  sample_files = Dir.glob(File.join(samples_dir, '*.md'))
                    .reject { |f| File.basename(f) == 'index.md' }

  # Extract title and filename for each sample
  samples_with_titles = sample_files.map do |sample_file|
    first_line = File.open(sample_file, &:readline).strip
    # Extract title from "# Title" format
    title = if first_line.start_with?('#')
              first_line.delete_prefix('#').strip
            else
              File.basename(sample_file, '.md')
            end
    [title, File.basename(sample_file)]
  end

  # Sort by title (case-insensitive)
  samples_with_titles.sort_by! { |title, _| title.downcase }

  # Build samples nav structure
  samples_nav = ["  - Samples:\n", "      - samples/index.md\n"]
  samples_with_titles.each do |title, filename|
    samples_nav << "      - #{title}: samples/#{filename}\n"
  end

  # Read mkdocs.yml
  lines = File.readlines(mkdocs_yml)

  # Find and replace the Samples section in nav
  new_lines = []
  skip_until_next_section = false

  lines.each do |line|
    # Detect if we're at the Samples section
    if line.match?(/^  - Samples:\s*$/)
      skip_until_next_section = true
      # Add the new samples navigation
      new_lines.concat(samples_nav)
      next
    end

    # Skip old samples entries
    if skip_until_next_section
      # Check if we've reached the next top-level nav item (same indentation as "- Samples:")
      if line.match?(/^  - [A-Z]/) && !line.strip.start_with?('- samples/')
        skip_until_next_section = false
        new_lines << line
      end
      # Skip lines that are part of the old samples section
      next
    end

    new_lines << line
  end

  # Write updated mkdocs.yml
  File.write(mkdocs_yml, new_lines.join)

  puts "Updated #{mkdocs_yml} with #{sample_files.length} sample pages in navigation"
end

main if __FILE__ == $PROGRAM_NAME
