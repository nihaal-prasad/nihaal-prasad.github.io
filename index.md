---
layout: default
title: Recent Blog Posts
---

<div class="home">
    <h1 class="page-heading">{{ page.title }}</h1>
    <hr>
    {%- if site.posts.size > 0 -%}
        {%- for post in site.posts limit: 5 -%}
            <h3>
                <a class="post-link" href="{{ post.url | relative_url }}">
                {{ post.title | escape }}
                </a>
            </h3>
            <div id="page-preview" style="margin-left: 5%">
                <p>{{ post.excerpt }}</p>
            </div>
            <hr>
        {%- endfor -%}
    {%- endif -%}
</div>
