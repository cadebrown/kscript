function Header (h)
  if h.identifier ~= '' then
    -- Insert a link icon at the beginning of the header content
    local icon = pandoc.Link({}, '#' .. h.identifier, nil, {class = 'anchor-link'})
    -- insert at the end
    table.insert(h.content, icon)
  end
  return h
end

function DefinitionList(dl)
  -- print(dl)
  -- local term = dl.content[1][1]
  -- print(term)

  for i, v in pairs(dl.content) do
    local term = v[1]

    -- first, start with just stringifying the content
    local id = pandoc.utils.stringify(term)

    -- cut it off at the first parenthesis
    id = id:match("^[^\\(]*")

    -- cut it off at the first colon
    id = id:match("^[^:]*")

    -- replace spaces and dashes with dashes
    id = id:gsub("%s+", "-")

    -- remove all non-alphanumeric characters except dashes and underscores and .
    id = id:gsub("[^%w-_%.]", "")

    -- print(id)

    term = pandoc.Span(term, id)

    local icon = pandoc.Link({}, '#' .. id, nil, {class = 'anchor-link'})

    table.insert(term.content, icon)
    v.identifier = id
    v[1] = term
  end
  return dl
end
