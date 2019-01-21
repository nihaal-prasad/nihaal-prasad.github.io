---
layout: default
title: Posts
---

<div class="posts">
  {%- if page.title -%}
    <h1 class="page-heading">List of Blog Posts</h1>
  {%- endif -%}
  <hr>
  {%- if site.posts.size > 0 -%}
    <ul class="post-list">
      {%- for post in site.posts -%}
      <li style="color: black;">
          <a class="post-link" href="{{ post.url | relative_url }}">
            {{ post.title | escape }}
          </a>
        {%- if site.show_excerpts -%}
          {{ post.excerpt }}
        {%- endif -%}
      </li>
      {%- endfor -%}
    </ul>
  {%- endif -%}

</div>

